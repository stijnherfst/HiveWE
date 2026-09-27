// Last, as it imports a module and textual standard library includes can't follow that
#include "vulkan_imgui.h"

import std;
import types;
import VkContext;
import VkTexture;

namespace {
	/// Mirrors PushConstants in data/shaders/imgui.*
	struct ImGuiPushConstants {
		float scale[2];
		float translate[2];
		uint32_t texture_slot;
		VkDeviceAddress vertices;
	};
	static_assert(offsetof(ImGuiPushConstants, vertices) == 24);
	static_assert(sizeof(ImDrawVert) == 20, "imgui.vert expects the default ImDrawVert layout");
} // namespace

VulkanImGuiRenderer::VulkanImGuiRenderer()
	: pipeline({
		  .vertex_shader = "data/shaders/imgui.vert.spv",
		  .fragment_shader = "data/shaders/imgui.frag.spv",
		  .blend = true,
		  .src_factor = VK_BLEND_FACTOR_SRC_ALPHA,
		  .dst_factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	  }) {}

VulkanImGuiRenderer::~VulkanImGuiRenderer() {
	release_font();
}

void VulkanImGuiRenderer::release_font() {
	if (font_image.image == VK_NULL_HANDLE || !vk_context.is_initialized()) {
		return;
	}
	bindless.remove(font_slot);
	destroy_image_deferred(font_image);
	font_image = {};
}

ImTextureID VulkanImGuiRenderer::create_font_texture(const unsigned char* pixels, const int width, const int height) {
	release_font();
	const VkExtent2D extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
	font_image = create_rgba8_image(extent, std::span(pixels, static_cast<size_t>(width) * height * 4));
	font_slot = bindless.add(font_image.view, bindless.samplers[0]);
	return static_cast<ImTextureID>(font_slot);
}

void VulkanImGuiRenderer::render(
	const VkCommandBuffer cmd,
	FrameAllocator& allocator,
	const ImDrawData* draw_data,
	const VkExtent2D framebuffer
) {
	if (!draw_data || draw_data->TotalVtxCount == 0) {
		return;
	}

	// All lists go into one vertex and one index allocation; each draw offsets into them
	const auto vertices = allocator.allocate(static_cast<VkDeviceSize>(draw_data->TotalVtxCount) * sizeof(ImDrawVert), 16);
	const auto indices = allocator.allocate(static_cast<VkDeviceSize>(draw_data->TotalIdxCount) * sizeof(ImDrawIdx), 16);
	{
		std::byte* vertex_cursor = vertices.data;
		std::byte* index_cursor = indices.data;
		for (const ImDrawList* list : draw_data->CmdLists) {
			std::memcpy(vertex_cursor, list->VtxBuffer.Data, list->VtxBuffer.Size * sizeof(ImDrawVert));
			std::memcpy(index_cursor, list->IdxBuffer.Data, list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vertex_cursor += list->VtxBuffer.Size * sizeof(ImDrawVert);
			index_cursor += list->IdxBuffer.Size * sizeof(ImDrawIdx);
		}
	}

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdSetCullMode(cmd, VK_CULL_MODE_NONE);
	vkCmdSetDepthTestEnable(cmd, VK_FALSE);
	vkCmdSetDepthWriteEnable(cmd, VK_FALSE);
	vkCmdBindIndexBuffer(cmd, indices.buffer, indices.offset, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

	// Maps ImGui's top-left origin, Y-down display coordinates to clip space, which is Y-up under the flipped viewport
	ImGuiPushConstants push = {.vertices = vertices.address};
	push.scale[0] = 2.f / draw_data->DisplaySize.x;
	push.scale[1] = -2.f / draw_data->DisplaySize.y;
	push.translate[0] = -1.f - draw_data->DisplayPos.x * push.scale[0];
	push.translate[1] = 1.f - draw_data->DisplayPos.y * push.scale[1];

	const VkViewport viewport = {
		0.f,
		static_cast<float>(framebuffer.height),
		static_cast<float>(framebuffer.width),
		-static_cast<float>(framebuffer.height),
		0.f,
		1.f,
	};
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	const ImVec2 clip_offset = draw_data->DisplayPos;
	const ImVec2 clip_scale = draw_data->FramebufferScale;

	int32_t vertex_offset = 0;
	uint32_t index_offset = 0;
	for (const ImDrawList* list : draw_data->CmdLists) {
		for (const ImDrawCmd& command : list->CmdBuffer) {
			if (command.UserCallback) {
				if (command.UserCallback != ImDrawCallback_ResetRenderState) {
					command.UserCallback(list, &command);
				}
				continue;
			}

			const float left = std::max((command.ClipRect.x - clip_offset.x) * clip_scale.x, 0.f);
			const float top = std::max((command.ClipRect.y - clip_offset.y) * clip_scale.y, 0.f);
			const float right = std::min((command.ClipRect.z - clip_offset.x) * clip_scale.x, static_cast<float>(framebuffer.width));
			const float bottom = std::min((command.ClipRect.w - clip_offset.y) * clip_scale.y, static_cast<float>(framebuffer.height));
			if (right <= left || bottom <= top) {
				continue;
			}
			const VkRect2D scissor = {
				{static_cast<int32_t>(left), static_cast<int32_t>(top)},
				{static_cast<uint32_t>(right - left), static_cast<uint32_t>(bottom - top)},
			};
			vkCmdSetScissor(cmd, 0, 1, &scissor);

			push.texture_slot = static_cast<uint32_t>(command.GetTexID());
			vkCmdPushConstants(cmd, bindless.pipeline_layout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);
			vkCmdDrawIndexed(
				cmd,
				command.ElemCount,
				1,
				index_offset + command.IdxOffset,
				vertex_offset + static_cast<int32_t>(command.VtxOffset),
				0
			);
		}
		vertex_offset += list->VtxBuffer.Size;
		index_offset += list->IdxBuffer.Size;
	}
}
