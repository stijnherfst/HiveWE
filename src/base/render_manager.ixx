module;

#include <volk.h>
#include <vk_mem_alloc.h>

export module RenderManager;

import std;
import types;
import SkinnedMesh;
import SkinnedMeshGlobals;
import Skeleton;
import ResourceManager;
import Timer;
import MDX;
import Camera;
import Utilities;
import Globals;
import Units;
import SLK;
import VkContext;
import VkResources;
import VkEditableMesh;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;
import Doodads;
import Doodad;

/// Mirrors SkinnedFrame in data/shaders/skinned_mesh_common.glsl
struct SkinnedFrameData {
	glm::mat4 VP;
	glm::vec4 light_direction;
	uint32_t render_lighting;
	uint32_t padding[3];
	VkDeviceAddress layer_colors;
	VkDeviceAddress uvs;
	VkDeviceAddress vertices;
	VkDeviceAddress tangents;
	VkDeviceAddress normals;
	VkDeviceAddress instance_matrices;
	VkDeviceAddress skins;
	VkDeviceAddress bone_matrices;
	VkDeviceAddress team_color_indexes;
	VkDeviceAddress draw_infos;
	VkDeviceAddress layer_textures;
	VkDeviceAddress layer_params;
};
static_assert(offsetof(SkinnedFrameData, layer_colors) == 96);

/// Mirrors PushConstants in data/shaders/skinned_mesh_common.glsl
struct SkinnedPushConstants {
	VkDeviceAddress frame;
	uint32_t draw_info_base;
	uint32_t shader_state;
};

/// Bits of SkinnedPushConstants::shader_state, the fixed-function state the fragment shader handles instead.
/// Mirrors data/shaders/skinned_mesh_fragment.glsl.
enum ShaderState : uint32_t {
	shader_cull = 1,
	shader_blend = 2,
};

/// Mirrors PushConstants in data/shaders/skinned_mesh_pick.*
struct PickPushConstants {
	glm::mat4 MVP;
	VkDeviceAddress bones;
	VkDeviceAddress vertices;
	VkDeviceAddress skins;
	int32_t color_id;
};

export class RenderManager {
	struct SkinnedInstance {
		SkinnedMesh* mesh;
		uint32_t instance_id;
		float distance;
	};

	/// A mesh drawn into the picking target with a color encoding its id
	struct PickJob {
		const SkinnedMesh* mesh;
		const Skeleton* skeleton;
		int id;
	};

	static constexpr VkFormat pick_color_format = VK_FORMAT_R8G8B8A8_UNORM;
	static constexpr VkFormat pick_depth_format = VK_FORMAT_D32_SFLOAT;

	std::vector<SkinnedMesh*> skinned_meshes;
	std::vector<SkinnedInstance> skinned_transparent_instances;

	// Per-frame scratch, kept between frames to reuse the allocations
	/// The queued meshes with geometry, in instance data order
	std::vector<SkinnedMesh*> frame_meshes;
	/// The first instance of each chunk the instance data is filled in
	std::vector<uint32_t> frame_chunks;
	std::vector<glm::vec4> frame_layer_colors;
	std::vector<SkinnedMeshGlobals::DrawIndexedIndirectCommand> transparent_commands;
	std::vector<SkinnedMeshGlobals::DrawInfo> transparent_draw_infos;
	std::vector<SkinnedMesh::DrawState> transparent_states;

	std::shared_ptr<SkinnedMesh> click_helper;
	Skeleton click_helper_skeleton;
	std::vector<glm::mat4> click_helper_matrices;

	// Indexed by BlendMode
	std::array<Pipeline, blend_mode_count> sd_pipelines;
	std::array<Pipeline, blend_mode_count> hd_pipelines;
	// Blend with `color * ONE + destination * factor`, where the fragment shader expresses the layer's blend mode
	Pipeline sd_dual_source_pipeline;
	Pipeline hd_dual_source_pipeline;

	Pipeline pick_pipeline {{
		.vertex_shader = "data/shaders/skinned_mesh_pick.vert.spv",
		.fragment_shader = "data/shaders/skinned_mesh_pick.frag.spv",
		.color_format = pick_color_format,
		.depth_format = pick_depth_format,
	}};
	Image pick_color;
	Image pick_depth;

	int window_width = 1;
	int window_height = 1;

  public:
	RenderManager() {
		skinned_mesh_globals.init();
		click_helper = resource_manager.load<SkinnedMesh>("Objects/InvalidObject/InvalidObject.mdx", "", std::nullopt).value();
		click_helper_skeleton = Skeleton(click_helper->mdx);
		click_helper_skeleton.update(0.016f);

		for (size_t mode = 0; mode < blend_mode_count; mode++) {
			const auto& [enable, src, dst] = blend_factors[mode];
			PipelineDescription description = {
				.blend = enable,
				.src_factor = src,
				.dst_factor = dst,
			};
			description.vertex_shader = "data/shaders/skinned_mesh_sd.vert.spv";
			description.fragment_shader = "data/shaders/skinned_mesh_sd.frag.spv";
			sd_pipelines[mode] = Pipeline(description);

			description.vertex_shader = "data/shaders/skinned_mesh_hd.vert.spv";
			description.fragment_shader = "data/shaders/skinned_mesh_hd.frag.spv";
			hd_pipelines[mode] = Pipeline(description);
		}

		PipelineDescription description = {
			.blend = true,
			.src_factor = VK_BLEND_FACTOR_ONE,
			.dst_factor = VK_BLEND_FACTOR_SRC1_COLOR,
		};
		description.vertex_shader = "data/shaders/skinned_mesh_sd.vert.spv";
		description.fragment_shader = "data/shaders/skinned_mesh_sd.frag.spv";
		sd_dual_source_pipeline = Pipeline(description);
		description.vertex_shader = "data/shaders/skinned_mesh_hd.vert.spv";
		description.fragment_shader = "data/shaders/skinned_mesh_hd.frag.spv";
		hd_dual_source_pipeline = Pipeline(description);
	}

	~RenderManager() {
		if (!vk_context.is_initialized()) {
			return;
		}
		destroy_image_deferred(pick_color);
		destroy_image_deferred(pick_depth);
	}

	void
	queue_render(SkinnedMesh& skinned_mesh, const Skeleton& skeleton, const glm::vec3 color, const uint32_t team_color_index) {
		queue_render(skinned_mesh, skeleton, skeleton.matrix, color, team_color_index);
	}

	/// Queues `skinned_mesh` posed by `skeleton` but placed by `matrix`, so instances in the same pose can share a skeleton
	void queue_render(
		SkinnedMesh& skinned_mesh,
		const Skeleton& skeleton,
		const glm::mat4& matrix,
		const glm::vec3 color,
		const uint32_t team_color_index
	) {
		if (in_view(skinned_mesh, skeleton, matrix)) {
			queue_render_in_view(skinned_mesh, skeleton, matrix, color, team_color_index);
		}
	}

	/// Whether `skinned_mesh` posed by `skeleton` and placed by `matrix` is inside the view frustum. Safe to call from
	/// several threads, so callers queueing many meshes can cull in parallel and pass the survivors to queue_render_in_view.
	bool in_view(const SkinnedMesh& skinned_mesh, const Skeleton& skeleton, const glm::mat4& matrix) const {
		const mdx::Extent& extent = skinned_mesh.mdx->sequences[skeleton.sequence_index].extent;
		return camera.inside_frustrum_transform(extent.minimum, extent.maximum, matrix);
	}

	/// queue_render for a mesh already known to be in view
	void queue_render_in_view(
		SkinnedMesh& skinned_mesh,
		const Skeleton& skeleton,
		const glm::mat4& matrix,
		const glm::vec3 color,
		const uint32_t team_color_index
	) {
		skinned_mesh.render_jobs.push_back(matrix);
		skinned_mesh.render_colors.push_back(color);
		skinned_mesh.render_team_color_indexes.push_back(team_color_index);
		skinned_mesh.skeletons.push_back(&skeleton);

		// Register for opaque drawing
		if (skinned_mesh.render_jobs.size() == 1) {
			skinned_meshes.push_back(&skinned_mesh);
		}

		// Register for transparent drawing
		// If the mesh contains transparent parts then those need to be sorted and drawn on top/after all the opaque parts
		if (skinned_mesh.geosets.empty()) {
			return;
		}

		if (skinned_mesh.has_transparent_layers) {
			skinned_transparent_instances.push_back(
				SkinnedInstance {
					.mesh = &skinned_mesh,
					.instance_id = static_cast<uint32_t>(skinned_mesh.render_jobs.size() - 1),
					.distance = glm::distance(camera.position - camera.direction * camera.distance, glm::vec3(matrix[3])),
				}
			);
		}
	}

	/// Renders a click helper (little purple checkered box) placed by `model`
	void queue_click_helper(const glm::mat4& model) {
		click_helper_matrices.push_back(model);
	}

	/// Draws everything queued since the last call and clears the queues.
	/// Expects rendering to have begun with the viewport set and the bindless set bound.
	void render(
		const VkCommandBuffer cmd,
		FrameAllocator& allocator,
		const bool render_lighting,
		const glm::vec3 light_direction
	) {
		for (const auto& matrix : click_helper_matrices) {
			queue_render(*click_helper, click_helper_skeleton, matrix, glm::vec3(1.f), 0);
		}

		// Per-frame data for all instances, laid out mesh by mesh. A serial pass over the meshes assigns their offsets and
		// a parallel pass over the instances fills them, as copying bones and evaluating layer colors is the bulk of the work.
		frame_meshes.clear();
		uint32_t instance_total = 0;
		uint32_t bone_total = 0;
		uint32_t layer_color_total = 0;
		for (auto* mesh : skinned_meshes) {
			if (mesh->geosets.empty()) {
				continue;
			}
			const uint32_t instance_count = static_cast<uint32_t>(mesh->render_jobs.size());
			mesh->frame_instance_offset = instance_total;
			mesh->frame_bone_offset = bone_total;
			mesh->frame_layer_color_offset = layer_color_total;
			frame_meshes.push_back(mesh);
			instance_total += instance_count;
			bone_total += instance_count * static_cast<uint32_t>(mesh->mdx->bones.size());
			layer_color_total += instance_count * static_cast<uint32_t>(mesh->skip_count);
		}

		if (instance_total > 0) {
			const auto allocate = [&](const VkDeviceSize size) {
				return size == 0 ? FrameAllocator::Allocation {} : allocator.allocate(size);
			};
			const FrameAllocator::Allocation instances = allocate(VkDeviceSize {instance_total} * sizeof(glm::mat4));
			const FrameAllocator::Allocation team_colors = allocate(VkDeviceSize {instance_total} * sizeof(uint32_t));
			const FrameAllocator::Allocation bones = allocate(VkDeviceSize {bone_total} * sizeof(glm::mat4));
			const FrameAllocator::Allocation layer_colors = allocate(VkDeviceSize {layer_color_total} * sizeof(glm::vec4));
			auto* const instance_data = reinterpret_cast<glm::mat4*>(instances.data);
			auto* const team_color_data = reinterpret_cast<uint32_t*>(team_colors.data);
			auto* const bone_data = reinterpret_cast<glm::mat4*>(bones.data);
			auto* const layer_color_data = reinterpret_cast<glm::vec4*>(layer_colors.data);
			// The transparent pass skips invisible layers, and reads the colors back from this copy as the GPU copy may
			// be uncached memory
			frame_layer_colors.resize(layer_color_total);

			constexpr uint32_t chunk_size = 256;
			frame_chunks.clear();
			for (uint32_t first = 0; first < instance_total; first += chunk_size) {
				frame_chunks.push_back(first);
			}
			std::for_each(std::execution::par, frame_chunks.begin(), frame_chunks.end(), [&](const uint32_t first) {
				const uint32_t last = std::min(first + chunk_size, instance_total);
				// The mesh holding instance `first`, then walked forward as the chunk crosses into later meshes
				auto mesh_it = std::ranges::upper_bound(frame_meshes, first, {}, &SkinnedMesh::frame_instance_offset) - 1;
				for (uint32_t index = first; index < last; index++) {
					while (index >= (*mesh_it)->frame_instance_offset + (*mesh_it)->render_jobs.size()) {
						++mesh_it;
					}
					const SkinnedMesh& mesh = **mesh_it;
					const uint32_t k = index - mesh.frame_instance_offset;
					const Skeleton& skeleton = *mesh.skeletons[k];

					instance_data[index] = mesh.render_jobs[k];
					team_color_data[index] = mesh.render_team_color_indexes[k];

					if (const size_t bone_count = mesh.mdx->bones.size(); bone_count > 0) {
						std::memcpy(
							bone_data + mesh.frame_bone_offset + k * bone_count,
							skeleton.world_matrices.data(),
							bone_count * sizeof(glm::mat4)
						);
					}

					size_t color_index = mesh.frame_layer_color_offset + k * static_cast<size_t>(mesh.skip_count);
					for (const auto& g : mesh.geosets) {
						glm::vec3 c = mesh.render_colors[k];
						float vis = 1.0f;
						if (g.geoset_anim && skeleton.sequence_index >= 0) {
							c *= skeleton.get_geoset_animation_color(*g.geoset_anim);
							vis = skeleton.get_geoset_animation_visiblity(*g.geoset_anim);
						}
						for (const auto& l : mesh.mdx->materials[g.material_id].layers) {
							const float lv = skeleton.sequence_index >= 0 ? skeleton.get_layer_visiblity(l) : 1.0f;
							const glm::vec4 layer_color(c, lv * vis);
							frame_layer_colors[color_index] = layer_color;
							layer_color_data[color_index] = layer_color;
							color_index++;
						}
					}
				}
			});

			const auto& g = skinned_mesh_globals;
			SkinnedFrameData frame = {
				.VP = camera.projection_view,
				.light_direction = glm::vec4(light_direction, 0.f),
				.render_lighting = render_lighting ? 1u : 0u,
				.layer_colors = layer_colors.address,
				.uvs = g.uv_snorm_buffer.address,
				.vertices = g.vertex_snorm_buffer.address,
				.tangents = g.tangent_buffer.address,
				.normals = g.normal_buffer.address,
				.instance_matrices = instances.address,
				.skins = g.weight_buffer.address,
				.bone_matrices = bones.address,
				.team_color_indexes = team_colors.address,
				.layer_textures = g.layer_texture_ids_buffer.address,
				.layer_params = g.layer_params_buffer.address,
			};

			vkCmdBindIndexBuffer(cmd, g.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);

			// OPAQUE PASS — collapse multi-draw across meshes within each draw-state group.
			render_opaque(cmd, allocator, frame, false);
			render_opaque(cmd, allocator, frame, true);

			// TRANSPARENT PASS — distance-sorted, coalesce adjacent same-state.
			std::sort(std::execution::par, skinned_transparent_instances.begin(), skinned_transparent_instances.end(), [](auto& l, auto& r) {
				return l.distance > r.distance;
			});
			render_transparent(cmd, allocator, frame, false);
			render_transparent(cmd, allocator, frame, true);
		}

		for (auto* m : skinned_meshes) {
			m->clear_render_data();
		}
		click_helper_matrices.clear();
		skinned_meshes.clear();
		skinned_transparent_instances.clear();
	}

	/// The map view's size in logical pixels, which mouse positions are in
	glm::ivec2 viewport_size() const {
		return {window_width, window_height};
	}

	void resize_framebuffers(const int width, const int height) {
		window_width = std::max(width, 1);
		window_height = std::max(height, 1);
	}

	/// Returns the unit ID of the unit that is currently under the mouse coordinates.
	/// Renders the meshes currently inside the view frustrum coded by unit ID and then reads the pixel under the mouse coordinates
	[[nodiscard]]
	std::optional<size_t> pick_unit_id_under_mouse(const Units& units, const glm::vec2 mouse_position) {
		std::vector<PickJob> jobs;
		for (size_t i = 0; i < units.units.size(); i++) {
			const Unit& unit = units.units[i];
			if (unit.id == "sloc") {
				continue;
			} // ToDo handle starting locations

			// TODO: technically we don't care about the frustrum. The mouse world ray just has to intersect with the AABB
			const mdx::Extent& extent = unit.mesh->mdx->sequences[unit.skeleton.sequence_index].extent;
			if (camera.inside_frustrum_transform(extent.minimum, extent.maximum, unit.skeleton.matrix)) {
				jobs.push_back({unit.mesh.get(), &unit.skeleton, static_cast<int>(i + 1)});
			}
		}

		return pick(jobs, mouse_position);
	}

	/// Returns the doodad ID of the doodad that is currently under the mouse coordinates.
	/// Renders the meshes currently inside the view frustrum coded by doodad ID and then reads the pixel under the mouse coordinates
	[[nodiscard]]
	std::optional<size_t> pick_doodad_id_under_mouse(const Doodads& doodads, const glm::vec2 mouse_position) {
		glm::vec3 window = {input_handler.mouse.x, window_height - input_handler.mouse.y, 1.f};
		glm::vec3 pos = glm::unProject(window, camera.view, camera.projection, glm::vec4(0, 0, window_width, window_height));
		glm::vec3 ray_origin = camera.position - camera.direction * camera.distance;
		glm::vec3 ray_direction = glm::normalize(pos - ray_origin);

		// Click helper skeletons must outlive the pick
		std::vector<std::unique_ptr<Skeleton>> helper_skeletons;
		std::vector<PickJob> jobs;
		for (size_t i = 0; i < doodads.doodads.size(); i++) {
			const Doodad& doodad = doodads.doodads[i];

			const mdx::Extent& extent = doodad.mesh->mdx->sequences[doodad.skeleton.sequence_index].extent;
			glm::vec3 local_min = extent.minimum;
			glm::vec3 local_max = extent.maximum;

			const bool use_click_helper = doodad.use_click_helper;
			if (use_click_helper) {
				local_min = glm::min(local_min, click_helper->mdx->extent.minimum);
				local_max = glm::max(local_max, click_helper->mdx->extent.maximum);
			}

			glm::vec3 min;
			glm::vec3 max;
			// From local space to world space
			transform_aabb_non_uniform(local_min, local_max, min, max, doodad.skeleton.matrix);

			if (intersect_aabb(min, max, ray_origin, ray_direction)) {
				jobs.push_back({doodad.mesh.get(), &doodad.skeleton, static_cast<int>(i + 1)});

				if (use_click_helper) {
					auto helper = std::make_unique<Skeleton>(click_helper->mdx);
					helper->matrix = doodad.skeleton.matrix;
					helper->update(0.016f);
					jobs.push_back({click_helper.get(), helper.get(), static_cast<int>(i + 1)});
					helper_skeletons.push_back(std::move(helper));
				}
			}
		}

		return pick(jobs, mouse_position);
	}

  private:
	template <typename T>
	static VkDeviceAddress upload_or_zero(FrameAllocator& allocator, const std::vector<T>& data) {
		if (data.empty()) {
			return 0;
		}
		return allocator.upload(std::span<const T>(data)).address;
	}

	VkPipeline pipeline_for(const bool render_hd, const uint8_t blend_mode) const {
		return (render_hd ? hd_pipelines : sd_pipelines)[blend_mode];
	}

	/// Writes a frame block pointing at this pass's draw infos, and the indirect commands, and returns their addresses
	std::pair<VkDeviceAddress, FrameAllocator::Allocation> write_pass(
		FrameAllocator& allocator,
		SkinnedFrameData frame,
		const std::vector<SkinnedMeshGlobals::DrawIndexedIndirectCommand>& commands,
		const std::vector<SkinnedMeshGlobals::DrawInfo>& draw_infos
	) const {
		frame.draw_infos = allocator.upload(std::span<const SkinnedMeshGlobals::DrawInfo>(draw_infos)).address;
		const auto frame_allocation = allocator.allocate(sizeof(SkinnedFrameData));
		std::memcpy(frame_allocation.data, &frame, sizeof(frame));
		const auto indirect = allocator.upload(std::span<const SkinnedMeshGlobals::DrawIndexedIndirectCommand>(commands), 16);
		return {frame_allocation.address, indirect};
	}

	void draw_group(
		const VkCommandBuffer cmd,
		const FrameAllocator::Allocation& indirect,
		const VkDeviceAddress frame_address,
		const size_t group_start,
		const size_t group_end,
		const uint32_t shader_state = 0
	) const {
		const SkinnedPushConstants push = {frame_address, static_cast<uint32_t>(group_start), shader_state};
		vkCmdPushConstants(cmd, bindless.pipeline_layout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);
		vkCmdDrawIndexedIndirect(
			cmd,
			indirect.buffer,
			indirect.offset + group_start * sizeof(SkinnedMeshGlobals::DrawIndexedIndirectCommand),
			static_cast<uint32_t>(group_end - group_start),
			sizeof(SkinnedMeshGlobals::DrawIndexedIndirectCommand)
		);
	}

	void render_opaque(
		const VkCommandBuffer cmd,
		FrameAllocator& allocator,
		const SkinnedFrameData& frame,
		const bool render_hd
	) const {
		std::vector<SkinnedMeshGlobals::DrawIndexedIndirectCommand> commands;
		std::vector<SkinnedMeshGlobals::DrawInfo> draw_infos;
		std::vector<SkinnedMesh::DrawState> states;

		for (const auto* mesh : frame_meshes) {
			const auto& entries = render_hd ? mesh->opaque_entries_hd : mesh->opaque_entries_sd;
			const uint32_t instance_count = static_cast<uint32_t>(mesh->render_jobs.size());
			const uint32_t bone_count = static_cast<uint32_t>(mesh->mdx->bones.size());
			const uint32_t skip_count = static_cast<uint32_t>(mesh->skip_count);

			for (const auto& e : entries) {
				commands.push_back({
					.indexCount = e.count,
					.instanceCount = instance_count,
					.firstIndex = e.first_index,
					.vertexOffset = e.base_vertex,
					.firstInstance = 0,
				});
				SkinnedMeshGlobals::DrawInfo di {};
				di.instance_offset = mesh->frame_instance_offset;
				di.bone_offset = mesh->frame_bone_offset;
				di.bone_count = bone_count;
				di.layer_color_offset = mesh->frame_layer_color_offset;
				di.layer_skip_count = skip_count;
				di.layer_index_global = e.layer_index_global;
				di.layer_index_local = e.layer_index_local;
				draw_infos.push_back(di);
				states.push_back(e.state);
			}
		}

		if (commands.empty()) {
			return;
		}

		// Sort (commands, draw_infos) jointly by DrawState so each state becomes one multi-draw
		std::vector<size_t> perm(commands.size());
		std::iota(perm.begin(), perm.end(), 0u);
		std::ranges::sort(perm, [&](const size_t a, const size_t b) {
			return states[a] < states[b];
		});

		std::vector<SkinnedMeshGlobals::DrawIndexedIndirectCommand> sorted_commands(commands.size());
		std::vector<SkinnedMeshGlobals::DrawInfo> sorted_infos(commands.size());
		std::vector<SkinnedMesh::DrawState> sorted_states(commands.size());
		for (size_t i = 0; i < perm.size(); i++) {
			sorted_commands[i] = commands[perm[i]];
			sorted_infos[i] = draw_infos[perm[i]];
			sorted_states[i] = states[perm[i]];
		}

		const auto [frame_address, indirect] = write_pass(allocator, frame, sorted_commands, sorted_infos);

		size_t group_start = 0;
		while (group_start < sorted_commands.size()) {
			size_t group_end = group_start + 1;
			while (group_end < sorted_commands.size() && sorted_states[group_end] == sorted_states[group_start]) {
				group_end++;
			}
			const auto& s = sorted_states[group_start];
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_for(render_hd, s.blend_mode));
			vkCmdSetCullMode(cmd, s.cull_face ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE);
			vkCmdSetDepthTestEnable(cmd, s.depth_test);
			vkCmdSetDepthWriteEnable(cmd, s.depth_mask);
			draw_group(cmd, indirect, frame_address, group_start, group_end);
			group_start = group_end;
		}
	}

	void render_transparent(
		const VkCommandBuffer cmd,
		FrameAllocator& allocator,
		const SkinnedFrameData& frame,
		const bool render_hd
	) {
		auto& commands = transparent_commands;
		auto& draw_infos = transparent_draw_infos;
		auto& states = transparent_states;
		commands.clear();
		draw_infos.clear();
		states.clear();

		for (const auto& inst : skinned_transparent_instances) {
			const SkinnedMesh* mesh = inst.mesh;
			const auto& entries = render_hd ? mesh->transparent_entries_hd : mesh->transparent_entries_sd;
			if (entries.empty()) {
				continue;
			}
			const uint32_t instance_offset = mesh->frame_instance_offset;
			const uint32_t bone_offset = mesh->frame_bone_offset;
			const uint32_t layer_color_offset = mesh->frame_layer_color_offset;
			const uint32_t bone_count = static_cast<uint32_t>(mesh->mdx->bones.size());
			const uint32_t skip_count = static_cast<uint32_t>(mesh->skip_count);
			const uint32_t instance_id = inst.instance_id;

			for (const auto& e : entries) {
				const size_t color_idx = layer_color_offset + instance_id * skip_count + e.layer_index_local;
				if (frame_layer_colors[color_idx].a <= 0.01f) {
					continue;
				}
				commands.push_back({
					.indexCount = e.count,
					.instanceCount = 1u,
					.firstIndex = e.first_index,
					.vertexOffset = e.base_vertex,
					.firstInstance = 0,
				});
				SkinnedMeshGlobals::DrawInfo di {};
				di.instance_offset = instance_offset + instance_id;
				di.bone_offset = bone_offset + instance_id * bone_count;
				di.bone_count = bone_count;
				di.layer_color_offset = layer_color_offset + instance_id * skip_count;
				di.layer_skip_count = skip_count;
				di.layer_index_global = e.layer_index_global;
				di.layer_index_local = e.layer_index_local;
				draw_infos.push_back(di);
				states.push_back(e.state);
			}
		}

		if (commands.empty()) {
			return;
		}

		const auto [frame_address, indirect] = write_pass(allocator, frame, commands, draw_infos);

		// The fragment shader culls back faces and blends, so draws only need splitting where depth testing changes or
		// a layer's blend mode needs a fixed-function pipeline. Transparent layers never write depth.
		struct GroupState {
			/// The dual-source pipeline, or else blend_mode's
			bool shader_blend;
			uint8_t blend_mode;
			bool depth_test;
			bool operator==(const GroupState&) const = default;
		};
		const auto group_state = [](const SkinnedMesh::DrawState& s) {
			const bool shader_blend = s.blend_mode != blend_modulate2x;
			return GroupState {
				.shader_blend = shader_blend,
				.blend_mode = shader_blend ? uint8_t {0} : s.blend_mode,
				.depth_test = s.depth_test,
			};
		};

		// Walk in order (distance-sorted) and coalesce adjacent runs that share state
		size_t group_start = 0;
		while (group_start < commands.size()) {
			const GroupState state = group_state(states[group_start]);
			size_t group_end = group_start + 1;
			while (group_end < commands.size() && group_state(states[group_end]) == state) {
				group_end++;
			}
			const VkPipeline pipeline = state.shader_blend
				? (render_hd ? hd_dual_source_pipeline : sd_dual_source_pipeline)
				: pipeline_for(render_hd, state.blend_mode);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			vkCmdSetCullMode(cmd, VK_CULL_MODE_NONE);
			vkCmdSetDepthTestEnable(cmd, state.depth_test);
			vkCmdSetDepthWriteEnable(cmd, VK_FALSE);
			const uint32_t shader_state = shader_cull | (state.shader_blend ? shader_blend : 0u);
			draw_group(cmd, indirect, frame_address, group_start, group_end, shader_state);
			group_start = group_end;
		}
	}

	/// (Re)creates the picking target at the window's size
	void ensure_pick_target() {
		const VkExtent2D extent = {static_cast<uint32_t>(window_width), static_cast<uint32_t>(window_height)};
		if (pick_color.extent.width == extent.width && pick_color.extent.height == extent.height) {
			return;
		}
		destroy_image_deferred(pick_color);
		destroy_image_deferred(pick_depth);
		pick_color = create_image(
			pick_color_format,
			extent,
			1,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
		pick_depth = create_image(pick_depth_format, extent, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	/// Draws the jobs color coded by id and returns the id under the mouse, if any. Blocks until the GPU is done.
	std::optional<size_t> pick(const std::vector<PickJob>& jobs, const glm::vec2 mouse_position) {
		const int x = static_cast<int>(mouse_position.x);
		const int y = static_cast<int>(mouse_position.y);
		if (jobs.empty() || x < 0 || y < 0 || x >= window_width || y >= window_height) {
			return std::nullopt;
		}

		ensure_pick_target();

		// All bone matrices for the jobs, in one host buffer the shaders read directly
		size_t total_bones = 0;
		for (const auto& job : jobs) {
			total_bones += std::max<size_t>(job.mesh->mdx->bones.size(), 1);
		}
		Buffer bones = create_buffer(total_bones * sizeof(glm::mat4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
		std::vector<VkDeviceAddress> bone_addresses;
		size_t bone_offset = 0;
		for (const auto& job : jobs) {
			const size_t count = job.mesh->mdx->bones.size();
			std::memcpy(static_cast<glm::mat4*>(bones.mapped) + bone_offset, job.skeleton->world_matrices.data(), count * sizeof(glm::mat4));
			bone_addresses.push_back(bones.address + bone_offset * sizeof(glm::mat4));
			bone_offset += std::max<size_t>(count, 1);
		}

		const Buffer readback = create_buffer(4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);

		vk_context.immediate_submit([&](const VkCommandBuffer cmd) {
			image_barrier(
				cmd,
				pick_color.image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
			);
			image_barrier(
				cmd,
				pick_depth.image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
				VK_ACCESS_2_NONE,
				VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_ASPECT_DEPTH_BIT
			);

			const VkRenderingAttachmentInfo color = {
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.imageView = pick_color.view,
				.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.clearValue = {.color = {{0.f, 0.f, 0.f, 1.f}}},
			};
			const VkRenderingAttachmentInfo depth = {
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.imageView = pick_depth.view,
				.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.clearValue = {.depthStencil = {1.f, 0}},
			};
			const VkRenderingInfo rendering = {
				.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
				.renderArea = {{0, 0}, pick_color.extent},
				.layerCount = 1,
				.colorAttachmentCount = 1,
				.pColorAttachments = &color,
				.pDepthAttachment = &depth,
			};
			vkCmdBeginRendering(cmd, &rendering);
			set_viewport(cmd, {{0, 0}, pick_color.extent}, pick_color.extent);
			vkCmdSetPolygonModeEXT(cmd, VK_POLYGON_MODE_FILL);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pick_pipeline);
			vkCmdBindIndexBuffer(cmd, skinned_mesh_globals.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);

			for (size_t j = 0; j < jobs.size(); j++) {
				const SkinnedMesh& mesh = *jobs[j].mesh;
				const Skeleton& skeleton = *jobs[j].skeleton;

				const PickPushConstants push = {
					.MVP = camera.projection_view * skeleton.matrix,
					.bones = bone_addresses[j],
					.vertices = skinned_mesh_globals.vertex_snorm_buffer.address,
					.skins = skinned_mesh_globals.weight_buffer.address,
					.color_id = jobs[j].id,
				};
				vkCmdPushConstants(cmd, bindless.pipeline_layout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);

				// The first visible layer of each geoset decides its depth and culling state
				for (const auto& geoset : mesh.geosets) {
					float geoset_anim_visibility = 1.0f;
					if (geoset.geoset_anim && skeleton.sequence_index >= 0) {
						geoset_anim_visibility = skeleton.get_geoset_animation_visiblity(*geoset.geoset_anim);
					}

					for (const auto& layer : mesh.mdx->materials[geoset.material_id].layers) {
						const float layer_visibility = skeleton.sequence_index >= 0 ? skeleton.get_layer_visiblity(layer) : 1.0f;
						if (layer_visibility * geoset_anim_visibility <= 0.001f) {
							continue;
						}

						vkCmdSetDepthTestEnable(cmd, !(layer.shading_flags & 0x40));
						vkCmdSetDepthWriteEnable(cmd, !(layer.shading_flags & 0x80));
						vkCmdSetCullMode(cmd, (layer.shading_flags & 0x10) ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT);
						vkCmdDrawIndexed(
							cmd,
							geoset.indices,
							1,
							geoset.base_index + mesh.index_base,
							geoset.base_vertex + static_cast<int32_t>(mesh.vertex_base),
							0
						);
						break;
					}
				}
			}
			vkCmdEndRendering(cmd);

			image_barrier(
				cmd,
				pick_color.image,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_ACCESS_2_TRANSFER_READ_BIT
			);
			// The picking target is top-left origin, like the mouse coordinates
			const VkBufferImageCopy region = {
				.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
				.imageOffset = {x, y, 0},
				.imageExtent = {1, 1, 1},
			};
			vkCmdCopyImageToBuffer(cmd, pick_color.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 1, &region);
		});

		vmaInvalidateAllocation(vk_context.allocator, readback.allocation, 0, VK_WHOLE_SIZE);
		glm::u8vec4 color;
		std::memcpy(&color, readback.mapped, sizeof(color));
		vmaDestroyBuffer(vk_context.allocator, readback.buffer, readback.allocation);
		vmaDestroyBuffer(vk_context.allocator, bones.buffer, bones.allocation);

		const int index = color.r + (color.g << 8) + (color.b << 16);
		if (index != 0) {
			return {index - 1};
		}
		return {};
	}
};
