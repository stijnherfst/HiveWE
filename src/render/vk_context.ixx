module;

#include <cstdlib>
#include <volk.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

export module VkContext;

import std;

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* data,
	void* user_data
);

/// Owns the Vulkan instance, device, graphics queue and memory allocator shared by every viewport.
/// The floor is Vulkan 1.3: dynamic rendering and synchronization2 are core there, and it is what MoltenVK exposes.
/// Newer functionality is enabled per extension when the device has it, never by raising the API version.
export class VkContext {
  public:
	vkb::Instance instance;
	vkb::PhysicalDevice physical_device;
	vkb::Device device;
	VkQueue graphics_queue = VK_NULL_HANDLE;
	uint32_t graphics_queue_family = 0;
	VmaAllocator allocator = VK_NULL_HANDLE;

	/// Optional extensions, enabled when present
	bool has_push_descriptor = false;
	/// VK_KHR_present_id and VK_KHR_present_wait, to wait until a presented frame is on screen
	bool has_present_wait = false;

	std::atomic<uint32_t> validation_errors = 0;
	std::atomic<uint32_t> validation_warnings = 0;

  private:
	// Every submit to graphics_queue goes through submit(), which signals the next value on this timeline.
	// Deferred destruction compares against it to know when the GPU is done with a resource.
	VkSemaphore timeline = VK_NULL_HANDLE;
	uint64_t last_submitted = 0;
	std::mutex queue_mutex;

	struct Garbage {
		uint64_t submission;
		std::move_only_function<void()> destroy;
	};
	std::vector<Garbage> garbage;
	std::mutex garbage_mutex;

	VkCommandPool immediate_pool = VK_NULL_HANDLE;
	VkFence immediate_fence = VK_NULL_HANDLE;
	std::mutex immediate_mutex;

  public:

	[[nodiscard]]
	bool is_initialized() const {
		return device.device != VK_NULL_HANDLE;
	}

	std::expected<void, std::string> init(const bool enable_validation) {
		if (is_initialized()) {
			return {};
		}

#ifdef HIVEWE_VULKAN_LAYER_PATH
		// The loader reads this when enumerating layers, so it only has to be set before the instance is created
		if (enable_validation) {
	#ifdef _WIN32
			size_t existing_length = 0;
			getenv_s(&existing_length, nullptr, 0, "VK_ADD_LAYER_PATH");
			if (existing_length == 0) {
				_putenv_s("VK_ADD_LAYER_PATH", HIVEWE_VULKAN_LAYER_PATH);
			}
	#else
			setenv("VK_ADD_LAYER_PATH", HIVEWE_VULKAN_LAYER_PATH, 0);
	#endif
		}
#endif

		if (volkInitialize() != VK_SUCCESS) {
			return std::unexpected("No Vulkan loader found (vulkan-1.dll missing)");
		}

		if (enable_validation) {
			const auto system_info = vkb::SystemInfo::get_system_info(vkGetInstanceProcAddr);
			if (!system_info || !system_info->validation_layers_available) {
				std::println("Vulkan: validation was requested but VK_LAYER_KHRONOS_validation is not available");
			}
		}

		auto instance_result = vkb::InstanceBuilder(vkGetInstanceProcAddr)
								   .set_app_name("HiveWE")
								   .set_engine_name("HiveWE")
								   .require_api_version(1, 3, 0)
								   .request_validation_layers(enable_validation)
								   .set_debug_callback(debug_callback)
								   .set_debug_callback_user_data_pointer(this)
								   .build();
		if (!instance_result) {
			return std::unexpected(std::format("Creating Vulkan instance failed: {}", instance_result.error().message()));
		}
		instance = instance_result.value();
		volkLoadInstanceOnly(instance.instance);

		VkPhysicalDeviceFeatures features {};
		features.multiDrawIndirect = true;
		// The transparent pass expresses each layer's blend mode in the fragment shader
		features.dualSrcBlend = true;
		features.fillModeNonSolid = true;
		features.samplerAnisotropy = true;
		features.textureCompressionBC = true;

		VkPhysicalDeviceVulkan11Features features_11 {};
		features_11.shaderDrawParameters = true;

		VkPhysicalDeviceVulkan12Features features_12 {};
		features_12.bufferDeviceAddress = true;
		features_12.timelineSemaphore = true;
		features_12.descriptorIndexing = true;
		features_12.runtimeDescriptorArray = true;
		features_12.shaderSampledImageArrayNonUniformIndexing = true;
		features_12.descriptorBindingPartiallyBound = true;
		features_12.descriptorBindingSampledImageUpdateAfterBind = true;
		features_12.descriptorBindingUpdateUnusedWhilePending = true;
		features_12.descriptorBindingVariableDescriptorCount = true;
		features_12.scalarBlockLayout = true;

		VkPhysicalDeviceVulkan13Features features_13 {};
		features_13.dynamicRendering = true;
		features_13.synchronization2 = true;
		features_13.maintenance4 = true;
		// glslang compiles `discard` to OpDemoteToHelperInvocation when targeting Vulkan 1.3
		features_13.shaderDemoteToHelperInvocation = true;

		// No surface exists yet; each viewport checks present support for its own surface
		auto physical_result = vkb::PhysicalDeviceSelector(instance)
								   .set_minimum_version(1, 3)
								   .defer_surface_initialization()
								   .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
								   .set_required_features(features)
								   .set_required_features_11(features_11)
								   .set_required_features_12(features_12)
								   .set_required_features_13(features_13)
								   .add_required_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
								   .add_required_extension(VK_KHR_MAINTENANCE_5_EXTENSION_NAME)
								   .add_required_extension_features(VkPhysicalDeviceMaintenance5FeaturesKHR {
									   .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR,
									   .maintenance5 = VK_TRUE,
								   })
								   // Wireframe switches the polygon mode per draw instead of doubling every pipeline
								   .add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME)
								   .add_required_extension_features(VkPhysicalDeviceExtendedDynamicState3FeaturesEXT {
									   .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT,
									   .extendedDynamicState3PolygonMode = VK_TRUE,
								   })
								   .select();
		if (!physical_result) {
			return std::unexpected(std::format("No suitable Vulkan 1.3 GPU found: {}", physical_result.error().message()));
		}
		physical_device = physical_result.value();

		has_push_descriptor = physical_device.enable_extension_if_present(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
		has_present_wait =
			physical_device.enable_extensions_if_present({VK_KHR_PRESENT_ID_EXTENSION_NAME, VK_KHR_PRESENT_WAIT_EXTENSION_NAME})
			&& physical_device.enable_extension_features_if_present(VkPhysicalDevicePresentIdFeaturesKHR {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR,
				.presentId = VK_TRUE,
			})
			&& physical_device.enable_extension_features_if_present(VkPhysicalDevicePresentWaitFeaturesKHR {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR,
				.presentWait = VK_TRUE,
			});

		auto device_result = vkb::DeviceBuilder(physical_device).build();
		if (!device_result) {
			return std::unexpected(std::format("Creating Vulkan device failed: {}", device_result.error().message()));
		}
		device = device_result.value();
		volkLoadDevice(device.device);

		graphics_queue = device.get_queue(vkb::QueueType::graphics).value();
		graphics_queue_family = device.get_queue_index(vkb::QueueType::graphics).value();

		VmaAllocatorCreateInfo allocator_info {};
		allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
		allocator_info.instance = instance.instance;
		allocator_info.physicalDevice = physical_device.physical_device;
		allocator_info.device = device.device;

		VmaVulkanFunctions functions {};
		vmaImportVulkanFunctionsFromVolk(&allocator_info, &functions);
		allocator_info.pVulkanFunctions = &functions;

		if (vmaCreateAllocator(&allocator_info, &allocator) != VK_SUCCESS) {
			return std::unexpected("Creating the Vulkan memory allocator failed");
		}

		VkSemaphoreTypeCreateInfo timeline_type = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
			.initialValue = 0,
		};
		const VkSemaphoreCreateInfo timeline_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = &timeline_type,
		};
		vkCreateSemaphore(device.device, &timeline_info, nullptr, &timeline);

		const VkCommandPoolCreateInfo pool_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			.queueFamilyIndex = graphics_queue_family,
		};
		vkCreateCommandPool(device.device, &pool_info, nullptr, &immediate_pool);

		const VkFenceCreateInfo fence_info = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
		vkCreateFence(device.device, &fence_info, nullptr, &immediate_fence);

		const auto& properties = physical_device.properties;
		std::println(
			"Vulkan {}.{}.{} on {} (driver {:#x}), push descriptors: {}, present wait: {}",
			VK_API_VERSION_MAJOR(properties.apiVersion),
			VK_API_VERSION_MINOR(properties.apiVersion),
			VK_API_VERSION_PATCH(properties.apiVersion),
			properties.deviceName,
			properties.driverVersion,
			has_push_descriptor,
			has_present_wait
		);

		return {};
	}

	/// Submits to the graphics queue, additionally signaling the submission timeline.
	/// Thread-safe. `fence` may be VK_NULL_HANDLE.
	VkResult submit(const VkSubmitInfo2& submit_info, const VkFence fence) {
		std::lock_guard lock(queue_mutex);

		std::vector<VkSemaphoreSubmitInfo> signals(
			submit_info.pSignalSemaphoreInfos,
			submit_info.pSignalSemaphoreInfos + submit_info.signalSemaphoreInfoCount
		);
		signals.push_back({
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = timeline,
			.value = last_submitted + 1,
			.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		});

		VkSubmitInfo2 info = submit_info;
		info.signalSemaphoreInfoCount = static_cast<uint32_t>(signals.size());
		info.pSignalSemaphoreInfos = signals.data();

		const VkResult result = vkQueueSubmit2(graphics_queue, 1, &info, fence);
		if (result == VK_SUCCESS) {
			last_submitted++;
		}
		return result;
	}

	/// Thread-safe vkQueuePresentKHR
	VkResult present(const VkPresentInfoKHR& present_info) {
		std::lock_guard lock(queue_mutex);
		return vkQueuePresentKHR(graphics_queue, &present_info);
	}

	/// Thread-safe vkQueueWaitIdle
	void wait_queue_idle() {
		std::lock_guard lock(queue_mutex);
		vkQueueWaitIdle(graphics_queue);
	}

	/// Records commands with `record`, submits them and waits for them to finish. Thread-safe.
	/// For one-off work such as uploads; it stalls the calling thread.
	void immediate_submit(const std::function<void(VkCommandBuffer)>& record) {
		std::lock_guard lock(immediate_mutex);

		const VkCommandBufferAllocateInfo allocate_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = immediate_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		VkCommandBuffer cmd;
		vkAllocateCommandBuffers(device.device, &allocate_info, &cmd);

		const VkCommandBufferBeginInfo begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		vkBeginCommandBuffer(cmd, &begin_info);
		record(cmd);
		vkEndCommandBuffer(cmd);

		const VkCommandBufferSubmitInfo cmd_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = cmd,
		};
		const VkSubmitInfo2 submit_info = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmd_info,
		};
		submit(submit_info, immediate_fence);
		vkWaitForFences(device.device, 1, &immediate_fence, VK_TRUE, UINT64_MAX);
		vkResetFences(device.device, 1, &immediate_fence);
		vkFreeCommandBuffers(device.device, immediate_pool, 1, &cmd);
	}

	/// Runs `destroy` once the GPU has finished every submission made so far. Thread-safe.
	/// Does nothing if the context is already gone (static destruction at exit).
	void defer_destroy(std::move_only_function<void()> destroy) {
		if (!is_initialized()) {
			return;
		}
		uint64_t submission;
		{
			std::lock_guard lock(queue_mutex);
			submission = last_submitted;
		}
		std::lock_guard lock(garbage_mutex);
		garbage.push_back({submission, std::move(destroy)});
	}

	/// Destroys deferred resources the GPU is done with. Call once per frame.
	void collect_garbage() {
		uint64_t completed = 0;
		vkGetSemaphoreCounterValue(device.device, timeline, &completed);

		std::vector<Garbage> ready;
		{
			std::lock_guard lock(garbage_mutex);
			const auto split = std::ranges::partition(garbage, [&](const Garbage& g) {
				return g.submission > completed;
			});
			ready.assign(std::make_move_iterator(split.begin()), std::make_move_iterator(split.end()));
			garbage.erase(split.begin(), split.end());
		}
		for (auto& g : ready) {
			g.destroy();
		}
	}

	/// Every viewport must be destroyed before this runs
	void destroy() {
		if (!is_initialized()) {
			return;
		}
		vkDeviceWaitIdle(device.device);
		{
			std::lock_guard lock(garbage_mutex);
			for (auto& g : garbage) {
				g.destroy();
			}
			garbage.clear();
		}
		vkDestroyFence(device.device, immediate_fence, nullptr);
		vkDestroyCommandPool(device.device, immediate_pool, nullptr);
		vkDestroySemaphore(device.device, timeline, nullptr);
		vmaDestroyAllocator(allocator);
		allocator = VK_NULL_HANDLE;
		vkb::destroy_device(device);
		device = {};
		vkb::destroy_instance(instance);
		instance = {};
	}
};

/// Never destroyed, so resources released during static destruction at exit (such as the resource manager's cache)
/// can still reach it. The OS reclaims the device when the process ends.
export inline VkContext& vk_context = *new VkContext;

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	const VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	const VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT* data,
	void* user_data
) {
	auto* context = static_cast<VkContext*>(user_data);

	const char* label;
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		context->validation_errors += 1;
		label = "error";
	} else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		context->validation_warnings += 1;
		label = "warning";
	} else {
		// Info and verbose messages are loader chatter
		return VK_FALSE;
	}

	std::println("---------------");
	std::println("Vulkan {}: {}", label, data->pMessage);
	return VK_FALSE;
}
