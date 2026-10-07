#include <QCoreApplication>
#include <QExposeEvent>
#include <QPlatformSurfaceEvent>
#include <QResizeEvent>
#include <QScreen>
#include <QTimer>

#include <volk.h>
#include <vk_mem_alloc.h>

// Last, as it imports a module and textual standard library includes can't follow that
#include "vulkan_viewport.h"

import std;
import VkContext;

namespace {
	// Wraps the VkContext instance for Qt, which creates window surfaces with it. Qt adopts the instance and never destroys it.
	QVulkanInstance* qt_instance = nullptr;
} // namespace

bool initialize_vulkan() {
	if (qt_instance) {
		return true;
	}

#ifdef NDEBUG
	constexpr bool validation = false;
#else
	constexpr bool validation = true;
#endif
	if (auto result = vk_context.init(validation); !result) {
		std::println("Vulkan unavailable: {}", result.error());
		return false;
	}
	bindless.init();

	auto instance = std::make_unique<QVulkanInstance>();
	instance->setVkInstance(vk_context.instance.instance);
	if (!instance->create()) {
		std::println("Vulkan unavailable: QVulkanInstance::create failed ({})", static_cast<int>(instance->errorCode()));
		bindless.destroy();
		vk_context.destroy();
		return false;
	}
	qt_instance = instance.release();
	return true;
}

void shutdown_vulkan() {
	if (!qt_instance) {
		return;
	}
	vk_context.wait_queue_idle();
	vk_context.collect_garbage();
	delete qt_instance;
	qt_instance = nullptr;
	bindless.destroy();
	vk_context.destroy();
}

VulkanViewport::VulkanViewport(QWindow* parent) : QWindow(parent) {
	setSurfaceType(VulkanSurface);
	setVulkanInstance(qt_instance);

	const VkDevice device = vk_context.device.device;

	const VkCommandPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = vk_context.graphics_queue_family,
	};
	vkCreateCommandPool(device, &pool_info, nullptr, &command_pool);

	for (auto& frame : frames) {
		const VkCommandBufferAllocateInfo allocate_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		vkAllocateCommandBuffers(device, &allocate_info, &frame.cmd);

		const VkFenceCreateInfo fence_info = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};
		vkCreateFence(device, &fence_info, nullptr, &frame.in_flight);

		const VkSemaphoreCreateInfo semaphore_info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		vkCreateSemaphore(device, &semaphore_info, nullptr, &frame.image_acquired);
	}

	if (vk_context.has_calibrated_timestamps && FramePacer::supported()) {
		const VkQueryPoolCreateInfo query_info = {
			.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
			.queryType = VK_QUERY_TYPE_TIMESTAMP,
			.queryCount = frames_in_flight,
		};
		vkCreateQueryPool(device, &query_info, nullptr, &query_pool);
	}
}

VulkanViewport::~VulkanViewport() {
	wait_idle();
	destroy_swapchain();

	const VkDevice device = vk_context.device.device;
	vkDestroyQueryPool(device, query_pool, nullptr);
	for (auto& frame : frames) {
		vkDestroyFence(device, frame.in_flight, nullptr);
		vkDestroySemaphore(device, frame.image_acquired, nullptr);
	}
	vkDestroyCommandPool(device, command_pool, nullptr);
}

void VulkanViewport::wait_idle() const {
	// Frame fences alone are not enough: a present can still be waiting on a render_finished semaphore after
	// its frame's fence signaled, and only draining the queue covers that
	vk_context.wait_queue_idle();
}

bool VulkanViewport::event(QEvent* event) {
	switch (event->type()) {
		case QEvent::UpdateRequest:
			update_scheduled = false;
			render();
			return true;
		case QEvent::PlatformSurface:
			// Qt destroys the VkSurfaceKHR along with the native window, so the swapchain has to go first
			if (static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
				wait_idle();
				destroy_swapchain();
				surface = VK_NULL_HANDLE;
			}
			break;
		default:
			break;
	}
	return QWindow::event(event);
}

void VulkanViewport::schedule_update() {
	if (update_scheduled) {
		return;
	}
	update_scheduled = true;
	const auto post = [this] {
		// Low priority, so input that arrived meanwhile is delivered before the next frame is recorded
		QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest), Qt::LowEventPriority);
	};

	if (pacing_active()) {
		const int64_t now = FramePacer::now();
		const int64_t delay = pacer.next_start(now) - now;
		const int64_t delay_ns = delay * 1'000'000'000 / FramePacer::ticks_per_second();
		if (delay_ns > 500'000) {
			QTimer::singleShot(std::chrono::nanoseconds(delay_ns), Qt::PreciseTimer, this, post);
			return;
		}
		post();
		return;
	}

	if (swapchain.present_mode == VK_PRESENT_MODE_MAILBOX_KHR && frame_clock.isValid()) {
		const qreal refresh_rate = screen() ? screen()->refreshRate() : 60.0;
		const qint64 min_interval_ns = static_cast<qint64>(0.95 * 1e9 / std::max(refresh_rate, 1.0));
		const qint64 remaining_ns = min_interval_ns - frame_clock.nsecsElapsed();
		if (remaining_ns > 1'000'000) {
			QTimer::singleShot(std::chrono::nanoseconds(remaining_ns), Qt::PreciseTimer, this, post);
			return;
		}
	}
	post();
}

bool VulkanViewport::pacing_active() const {
	return query_pool != VK_NULL_HANDLE && swapchain.present_mode == VK_PRESENT_MODE_MAILBOX_KHR;
}

int64_t VulkanViewport::gpu_to_host(const uint64_t timestamp) {
	const int64_t ticks_per_second = FramePacer::ticks_per_second();
	if (calibration_host == 0 || FramePacer::now() - calibration_host > ticks_per_second) {
		const std::array<VkCalibratedTimestampInfoKHR, 2> infos = {{
			{.sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_KHR, .timeDomain = VK_TIME_DOMAIN_DEVICE_KHR},
			{.sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_KHR, .timeDomain = VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_KHR},
		}};
		std::array<uint64_t, 2> timestamps;
		uint64_t max_deviation;
		vkGetCalibratedTimestampsKHR(vk_context.device.device, 2, infos.data(), timestamps.data(), &max_deviation);
		calibration_gpu = timestamps[0];
		calibration_host = static_cast<int64_t>(timestamps[1]);
	}

	// Timestamps wrap at timestamp_valid_bits, so the difference is taken in those bits and sign extended
	const uint32_t unused_bits = 64 - vk_context.timestamp_valid_bits;
	const int64_t gpu_ticks = static_cast<int64_t>((timestamp - calibration_gpu) << unused_bits) >> unused_bits;
	const double nanoseconds = static_cast<double>(gpu_ticks) * vk_context.physical_device.properties.limits.timestampPeriod;
	return calibration_host + static_cast<int64_t>(nanoseconds * static_cast<double>(ticks_per_second) / 1e9);
}

void VulkanViewport::exposeEvent(QExposeEvent*) {
	if (isExposed()) {
		schedule_update();
	}
}

void VulkanViewport::resizeEvent(QResizeEvent*) {
	swapchain_dirty = true;
}

bool VulkanViewport::ensure_swapchain() {
	const VkDevice device = vk_context.device.device;

	if (surface == VK_NULL_HANDLE) {
		// Owned by Qt and valid until the platform window is destroyed
		surface = QVulkanInstance::surfaceForWindow(this);
		if (surface == VK_NULL_HANDLE) {
			std::println("VulkanViewport: Qt could not create a Vulkan surface for this window");
			return false;
		}

		VkBool32 supported = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(
			vk_context.physical_device.physical_device,
			vk_context.graphics_queue_family,
			surface,
			&supported
		);
		if (!supported) {
			std::println("VulkanViewport: the graphics queue cannot present to this window");
			return false;
		}
		swapchain_dirty = true;
	}

	if (!swapchain_dirty && swapchain.swapchain != VK_NULL_HANDLE) {
		return true;
	}

	const qreal ratio = devicePixelRatio();
	const uint32_t width = static_cast<uint32_t>(std::lround(size().width() * ratio));
	const uint32_t height = static_cast<uint32_t>(std::lround(size().height() * ratio));
	if (width == 0 || height == 0) {
		// Minimized or collapsed; nothing to present until the next expose
		return false;
	}

	wait_idle();

	auto result = vkb::SwapchainBuilder(
					  vk_context.physical_device.physical_device,
					  device,
					  surface,
					  vk_context.graphics_queue_family,
					  vk_context.graphics_queue_family
	)
					  .set_desired_format({color_attachment_format, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
					  .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
					  .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR)
					  .set_desired_extent(width, height)
					  .set_old_swapchain(swapchain)
					  .build();
	if (!result) {
		std::println("VulkanViewport: creating swapchain failed: {}", result.error().message());
		return false;
	}
	// vk-bootstrap falls back to another format when the desired one is missing, which no pipeline is built for
	if (result->image_format != color_attachment_format) {
		std::println("VulkanViewport: the surface does not support VK_FORMAT_B8G8R8A8_UNORM");
		vkb::destroy_swapchain(result.value());
		return false;
	}

	destroy_swapchain();
	swapchain = result.value();
	swapchain_images = swapchain.get_images().value();
	swapchain_views = swapchain.get_image_views().value();

	render_finished.resize(swapchain_images.size());
	for (auto& semaphore : render_finished) {
		const VkSemaphoreCreateInfo semaphore_info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		vkCreateSemaphore(device, &semaphore_info, nullptr, &semaphore);
	}

	for (size_t i = 0; i < swapchain_images.size(); i++) {
		depth_images.push_back(
			create_image(depth_attachment_format, swapchain.extent, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT)
		);
	}

	swapchain_first_present_id = present_id + 1;
	swapchain_dirty = false;
	pacer.reset();
	return true;
}

void VulkanViewport::destroy_swapchain() {
	const VkDevice device = vk_context.device.device;
	for (auto semaphore : render_finished) {
		vkDestroySemaphore(device, semaphore, nullptr);
	}
	render_finished.clear();
	for (const auto& depth : depth_images) {
		destroy_image_deferred(depth);
	}
	depth_images.clear();
	if (!swapchain_views.empty()) {
		swapchain.destroy_image_views(swapchain_views);
		swapchain_views.clear();
	}
	swapchain_images.clear();
	if (swapchain.swapchain != VK_NULL_HANDLE) {
		vkb::destroy_swapchain(swapchain);
		swapchain = {};
	}
}

void VulkanViewport::render() {
	if (!isExposed() || !ensure_swapchain()) {
		return;
	}

	const int64_t start = FramePacer::now();
	vk_context.collect_garbage();
	frame_clock.start();

	const VkDevice device = vk_context.device.device;
	Frame& frame = frames[frame_index];

	vkWaitForFences(device, 1, &frame.in_flight, VK_TRUE, UINT64_MAX);
	frame.allocator.reset();

	if (frame.timed) {
		frame.timed = false;
		uint64_t timestamp;
		const VkResult result = vkGetQueryPoolResults(
			device,
			query_pool,
			frame_index,
			1,
			sizeof(timestamp),
			&timestamp,
			sizeof(timestamp),
			VK_QUERY_RESULT_64_BIT
		);
		if (result == VK_SUCCESS) {
			pacer.frame_finished(frame.start, gpu_to_host(timestamp), frame.target);
		}
	}
	frame.start = pacer.frame_started(start);
	frame.target = pacer.target();

	uint32_t image_index;
	const VkResult acquired =
		vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, frame.image_acquired, VK_NULL_HANDLE, &image_index);
	if (acquired == VK_ERROR_OUT_OF_DATE_KHR) {
		swapchain_dirty = true;
		schedule_update();
		return;
	}
	if (acquired != VK_SUCCESS && acquired != VK_SUBOPTIMAL_KHR) {
		std::println("VulkanViewport: vkAcquireNextImageKHR failed ({})", static_cast<int>(acquired));
		return;
	}

	vkResetFences(device, 1, &frame.in_flight);
	vkResetCommandBuffer(frame.cmd, 0);

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(frame.cmd, &begin_info);
	if (query_pool != VK_NULL_HANDLE) {
		vkCmdResetQueryPool(frame.cmd, query_pool, frame_index, 1);
	}
	// Every pipeline takes the polygon mode as dynamic state; wireframe rendering switches it to LINE and back
	vkCmdSetPolygonModeEXT(frame.cmd, VK_POLYGON_MODE_FILL);

	const VkImage image = swapchain_images[image_index];
	// The acquire semaphore wait happens at COLOR_ATTACHMENT_OUTPUT, so the layout transition is chained to that stage
	image_barrier(
		frame.cmd,
		image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_2_NONE,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
	);

	const Image& depth = depth_images[image_index];
	// Waits for the previous frame's depth writes to this image before discarding its contents
	image_barrier(
		frame.cmd,
		depth.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT
	);

	record(
		frame.cmd,
		FrameTarget {
			.image = image,
			.view = swapchain_views[image_index],
			.extent = swapchain.extent,
			.depth_view = depth.view,
		},
		frame.allocator
	);

	image_barrier(
		frame.cmd,
		image,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_2_NONE
	);

	if (query_pool != VK_NULL_HANDLE) {
		vkCmdWriteTimestamp2(frame.cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, query_pool, frame_index);
		frame.timed = true;
	}

	vkEndCommandBuffer(frame.cmd);

	const VkSemaphoreSubmitInfo wait_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = frame.image_acquired,
		.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
	};
	const VkSemaphoreSubmitInfo signal_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = render_finished[image_index],
		.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
	};
	const VkCommandBufferSubmitInfo cmd_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = frame.cmd,
	};
	const VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.waitSemaphoreInfoCount = 1,
		.pWaitSemaphoreInfos = &wait_info,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &cmd_info,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = &signal_info,
	};
	vk_context.submit(submit, frame.in_flight);

	present_id++;
	const VkPresentIdKHR present_id_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_ID_KHR,
		.swapchainCount = 1,
		.pPresentIds = &present_id,
	};
	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = vk_context.has_present_wait ? &present_id_info : nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &render_finished[image_index],
		.swapchainCount = 1,
		.pSwapchains = &swapchain.swapchain,
		.pImageIndices = &image_index,
	};
	const VkResult presented = vk_context.present(present_info);
	if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_SUBOPTIMAL_KHR) {
		swapchain_dirty = true;
	}
	qt_instance->presentQueued(this);

	// Without this wait, FIFO present lets frames queue up for several refreshes, and the camera lags behind the mouse.
	// Under FIFO, waiting until the previous frame is on screen keeps at most one frame queued behind the display, while
	// the CPU records the next frame as the GPU draws this one. Waiting for this frame instead would leave the GPU idle
	// during recording, and then it lowers its clocks.
	// Under MAILBOX a newer frame replaces a queued one, so waiting for this frame starts the next one right at the
	// refresh that showed it, and input is read one refresh before it is shown.
	// FramePacer replaces this wait, as it schedules the next frame itself.
	// The timeout keeps an occluded window, which may never show the frame, from stalling the event loop.
	const bool mailbox = swapchain.present_mode == VK_PRESENT_MODE_MAILBOX_KHR;
	const uint64_t wait_id = mailbox ? present_id : present_id - 1;
	if (vk_context.has_present_wait && !pacing_active() && !swapchain_dirty && wait_id >= swapchain_first_present_id) {
		constexpr uint64_t timeout_ns = 50'000'000;
		vkWaitForPresentKHR(device, swapchain.swapchain, wait_id, timeout_ns);
	}

	presented_frames++;
	frame_index = (frame_index + 1) % frames_in_flight;
	schedule_update();
}

VulkanOverlay::VulkanOverlay()
	: pipeline({
		  .vertex_shader = "data/shaders/overlay.vert.spv",
		  .fragment_shader = "data/shaders/overlay.frag.spv",
		  .blend = true,
		  .src_factor = VK_BLEND_FACTOR_ONE,
		  .dst_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	  }) {}

VulkanOverlay::~VulkanOverlay() {
	release_gpu_image();
}

QImage& VulkanOverlay::begin_paint(const QSize logical_size, const qreal device_pixel_ratio) {
	const QSize pixel_size = (QSizeF(logical_size) * device_pixel_ratio).toSize();
	if (image.size() != pixel_size) {
		// Byte order B, G, R, A, which uploads as VK_FORMAT_B8G8R8A8_UNORM
		image = QImage(pixel_size, QImage::Format_ARGB32_Premultiplied);
	}
	image.setDevicePixelRatio(device_pixel_ratio);
	image.fill(Qt::transparent);
	dirty = true;
	visible = true;
	return image;
}

void VulkanOverlay::clear() {
	visible = false;
	dirty = false;
}

void VulkanOverlay::release_gpu_image() {
	if (gpu_image.image == VK_NULL_HANDLE || !vk_context.is_initialized()) {
		return;
	}
	bindless.remove(slot);
	destroy_image_deferred(gpu_image);
	gpu_image = {};
	gpu_layout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void VulkanOverlay::upload(const VkCommandBuffer cmd, FrameAllocator& allocator) {
	if (!dirty || image.isNull()) {
		return;
	}
	dirty = false;

	const VkExtent2D extent = {static_cast<uint32_t>(image.width()), static_cast<uint32_t>(image.height())};
	if (gpu_image.extent.width != extent.width || gpu_image.extent.height != extent.height) {
		release_gpu_image();
		gpu_image = create_image(
			VK_FORMAT_B8G8R8A8_UNORM,
			extent,
			1,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
		slot = bindless.add(gpu_image.view, bindless.samplers[0]);
	}

	const auto pixels = allocator.upload(std::span(image.constBits(), static_cast<size_t>(image.sizeInBytes())), 16);

	// Earlier frames on the queue may still be sampling the previous contents
	image_barrier(
		cmd,
		gpu_image.image,
		gpu_layout,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		VK_ACCESS_2_NONE,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		VK_ACCESS_2_TRANSFER_WRITE_BIT
	);
	const VkBufferImageCopy region = {
		.bufferOffset = pixels.offset,
		.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
		.imageExtent = {extent.width, extent.height, 1},
	};
	vkCmdCopyBufferToImage(cmd, pixels.buffer, gpu_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	image_barrier(
		cmd,
		gpu_image.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		VK_ACCESS_2_SHADER_SAMPLED_READ_BIT
	);
	gpu_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void VulkanOverlay::draw(const VkCommandBuffer cmd) const {
	if (!visible || gpu_layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		return;
	}
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdSetCullMode(cmd, VK_CULL_MODE_NONE);
	vkCmdSetDepthTestEnable(cmd, VK_FALSE);
	vkCmdSetDepthWriteEnable(cmd, VK_FALSE);
	vkCmdPushConstants(cmd, bindless.pipeline_layout, VK_SHADER_STAGE_ALL, 0, sizeof(slot), &slot);
	vkCmdDraw(cmd, 3, 1, 0, 0);
}
