#pragma once

#include <QElapsedTimer>
#include <QImage>
#include <QVulkanInstance>
#include <QWindow>

#include <volk.h>
#include <VkBootstrap.h>

#include <array>
#include <cstdint>
#include <vector>

import VkResources;

/// Creates the shared Vulkan device, the bindless table and the QVulkanInstance viewports present with.
/// Safe to call repeatedly; returns false (after printing why) if Vulkan is unusable.
/// Validation layers are enabled in debug builds.
bool initialize_vulkan();

/// Tears down what initialize_vulkan created. Every viewport and Vulkan resource must be gone first.
void shutdown_vulkan();

/// A Vulkan window that presents through its own swapchain on the shared VkContext device.
/// Embed it in a widget hierarchy with QWidget::createWindowContainer.
/// Renders continuously: each presented frame requests the next update. MAILBOX present is preferred, paced by present
/// wait and a frame cap near the refresh rate; FIFO is the fallback, paced by vsync. HIVEWE_PRESENT_MODE=fifo forces FIFO.
class VulkanViewport : public QWindow {
  public:
	/// The color image is in color_attachment_format and the depth image in depth_attachment_format
	struct FrameTarget {
		VkImage image;
		VkImageView view;
		VkExtent2D extent;
		VkImageView depth_view;
	};

	explicit VulkanViewport(QWindow* parent = nullptr);
	~VulkanViewport() override;

	[[nodiscard]]
	uint64_t frames_presented() const {
		return presented_frames;
	}

  protected:
	/// Records the frame. The color image is in COLOR_ATTACHMENT_OPTIMAL layout and must stay in it; the depth image is in
	/// DEPTH_ATTACHMENT_OPTIMAL with undefined contents, so clear it when beginning rendering. The polygon mode is FILL.
	/// `allocator` holds data for this frame only and is recycled once the frame is done.
	virtual void record(VkCommandBuffer cmd, const FrameTarget& target, FrameAllocator& allocator) = 0;

	/// Waits until the GPU and presentation engine are done with everything this viewport submitted.
	/// Only for teardown and swapchain recreation; it stalls every viewport sharing the queue.
	void wait_idle() const;

	/// Queues a render
	void schedule_update();

	bool event(QEvent* event) override;
	void exposeEvent(QExposeEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

  private:
	static constexpr uint32_t frames_in_flight = 2;

	struct Frame {
		VkCommandBuffer cmd = VK_NULL_HANDLE;
		VkFence in_flight = VK_NULL_HANDLE;
		VkSemaphore image_acquired = VK_NULL_HANDLE;
		FrameAllocator allocator;
	};

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	vkb::Swapchain swapchain;
	std::vector<VkImage> swapchain_images;
	std::vector<VkImageView> swapchain_views;
	std::vector<Image> depth_images;
	// One per swapchain image rather than per frame, since presentation holds on to it until the image is reacquired
	std::vector<VkSemaphore> render_finished;
	bool swapchain_dirty = true;
	bool update_scheduled = false;

	VkCommandPool command_pool = VK_NULL_HANDLE;
	std::array<Frame, frames_in_flight> frames;
	uint32_t frame_index = 0;
	uint64_t presented_frames = 0;
	/// Identifies each present to vkWaitForPresentKHR; increases across swapchains
	uint64_t present_id = 0;
	/// The first present_id given to the current swapchain, as waiting on an earlier id is only meaningful to the swapchain it was presented to
	uint64_t swapchain_first_present_id = 1;
	/// Measures the time since the last frame started, for the frame cap MAILBOX needs
	QElapsedTimer frame_clock;

	void render();
	bool ensure_swapchain();
	void destroy_swapchain();
};

/// Composites an image drawn with QPainter over a viewport, for text and 2D decorations that have no Vulkan renderer.
/// Repaint through begin_paint() only when the content changes; the last upload is reused otherwise.
class VulkanOverlay {
  public:
	VulkanOverlay();
	VulkanOverlay(const VulkanOverlay&) = delete;
	VulkanOverlay& operator=(const VulkanOverlay&) = delete;
	~VulkanOverlay();

	/// Returns a transparent image covering `logical_size` at `device_pixel_ratio`, to paint with QPainter in logical coordinates
	QImage& begin_paint(QSize logical_size, qreal device_pixel_ratio);

	/// Hides the overlay until the next begin_paint()
	void clear();

	/// Uploads a pending repaint. Must be recorded outside of dynamic rendering.
	void upload(VkCommandBuffer cmd, FrameAllocator& allocator);

	/// Draws the overlay over the whole render area. Must be recorded inside dynamic rendering, with the bindless set bound.
	void draw(VkCommandBuffer cmd) const;

  private:
	QImage image;
	bool dirty = false;
	bool visible = false;

	Image gpu_image;
	VkImageLayout gpu_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	uint32_t slot = 0;

	Pipeline pipeline;

	void release_gpu_image();
};
