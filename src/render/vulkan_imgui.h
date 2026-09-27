#pragma once

#include <imgui.h>
#include <volk.h>

#include <cstdint>

import VkResources;

/// Draws ImGui draw data with Vulkan. Texture IDs are bindless slots, so ImGui::Image takes a VulkanTexture's slot.
class VulkanImGuiRenderer {
  public:
	VulkanImGuiRenderer();
	VulkanImGuiRenderer(const VulkanImGuiRenderer&) = delete;
	VulkanImGuiRenderer& operator=(const VulkanImGuiRenderer&) = delete;
	~VulkanImGuiRenderer();

	/// Uploads the font atlas and returns its texture ID. Replaces any previous atlas.
	ImTextureID create_font_texture(const unsigned char* pixels, int width, int height);

	/// Records the draw data. Must be recorded inside dynamic rendering, with the bindless set bound.
	void render(VkCommandBuffer cmd, FrameAllocator& allocator, const ImDrawData* draw_data, VkExtent2D framebuffer);

  private:
	Image font_image;
	uint32_t font_slot = 0;

	Pipeline pipeline;

	void release_font();
};
