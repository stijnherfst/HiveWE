#include <QExposeEvent>
#include <QPlatformSurfaceEvent>
#include <QResizeEvent>

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
}

VulkanViewport::~VulkanViewport() {
	wait_idle();
	destroy_swapchain();

	const VkDevice device = vk_context.device.device;
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

void VulkanViewport::exposeEvent(QExposeEvent*) {
	if (isExposed()) {
		requestUpdate();
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
					  .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
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

	swapchain_dirty = false;
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

	vk_context.collect_garbage();

	const VkDevice device = vk_context.device.device;
	Frame& frame = frames[frame_index];

	vkWaitForFences(device, 1, &frame.in_flight, VK_TRUE, UINT64_MAX);
	frame.allocator.reset();

	uint32_t image_index;
	const VkResult acquired =
		vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, frame.image_acquired, VK_NULL_HANDLE, &image_index);
	if (acquired == VK_ERROR_OUT_OF_DATE_KHR) {
		swapchain_dirty = true;
		requestUpdate();
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
		VK_PIPELINE_STAGE_2_NONE,
		VK_ACCESS_2_NONE
	);

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
	// Waiting until this frame is on screen lets Qt deliver the input that arrived meanwhile before the next frame is recorded.
	// The timeout keeps an occluded window, which may never show the frame, from stalling the event loop.
	if (vk_context.has_present_wait && !swapchain_dirty) {
		constexpr uint64_t timeout_ns = 50'000'000;
		vkWaitForPresentKHR(device, swapchain.swapchain, present_id, timeout_ns);
	}

	presented_frames++;
	frame_index = (frame_index + 1) % frames_in_flight;
	requestUpdate();
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
