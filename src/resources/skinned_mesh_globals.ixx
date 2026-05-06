module;

#include <cassert>
#include <cstdint>
#include <glad/glad.h>

export module SkinnedMeshGlobals;

import std;

/// Shared mega-buffers for all SkinnedMesh static and per-frame data.
/// init_gl() must run on the main GL context before any worker thread submits a SkinnedMesh constructor.
/// reserve() is mutex-protected so worker threads can carve out non-overlapping ranges.
/// glNamedBufferSubData from worker threads into those non-overlapping ranges is well-defined; the
/// per-task glFlush() in gl_thread_pool ensures uploads are visible to the main context.
export class SkinnedMeshGlobals {
  public:
	struct Allocation {
		uint32_t vertex_base;
		uint32_t index_base;
		uint32_t layer_base;
		uint32_t texture_base;
	};

	// Mirrors std430 layout in skinned_mesh_*.vert - 32 bytes.
	struct DrawInfo {
		uint32_t instance_offset;
		uint32_t bone_offset;
		uint32_t bone_count;
		uint32_t layer_color_offset;
		uint32_t layer_skip_count;
		uint32_t layer_index_global;
		uint32_t layer_index_local;
		uint32_t _pad;
	};

	struct DrawElementsIndirectCommand {
		GLuint count;
		GLuint instanceCount;
		GLuint firstIndex;
		GLint baseVertex;
		GLuint baseInstance;
	};

	// Capacity sized large enough for any reasonable Warcraft III map. Reserve asserts on overflow.
	static constexpr uint32_t max_vertices = 4 * 1024 * 1024;
	static constexpr uint32_t max_indices = 12 * 1024 * 1024;
	static constexpr uint32_t max_layers = 4 * 1024;
	static constexpr uint32_t max_textures = 16 * 1024;

	// Static buffers (created in init_gl on the main thread, written via glNamedBufferSubData).
	GLuint vertex_snorm_buffer = 0;
	GLuint uv_snorm_buffer = 0;
	GLuint normal_buffer = 0;
	GLuint tangent_buffer = 0;
	GLuint weight_buffer = 0;
	GLuint index_buffer = 0;
	GLuint texture_handles_ssbo = 0;
	GLuint layer_texture_ids_ssbo = 0;
	GLuint layer_params_ssbo = 0;

	// Global VAO. element buffer = global index_buffer.
	GLuint vao = 0;

	// Per-frame buffers (orphan-and-reload via glNamedBufferData each frame).
	GLuint instance_ssbo = 0;
	GLuint instance_team_color_index_ssbo = 0;
	GLuint bone_matrices_ssbo = 0;
	GLuint layer_colors_ssbo = 0;
	GLuint indirect_buffer = 0;
	GLuint draw_infos_ssbo = 0;

	/// Must be called from the main GL context before any SkinnedMesh constructor runs
	void init_gl() {
		if (vao != 0) {
			return;
		}
		glCreateBuffers(1, &vertex_snorm_buffer);
		glNamedBufferStorage(vertex_snorm_buffer, max_vertices * sizeof(uint64_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glCreateBuffers(1, &uv_snorm_buffer);
		glNamedBufferStorage(uv_snorm_buffer, max_vertices * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glCreateBuffers(1, &normal_buffer);
		glNamedBufferStorage(normal_buffer, max_vertices * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glCreateBuffers(1, &tangent_buffer);
		glNamedBufferStorage(tangent_buffer, max_vertices * 16, nullptr, GL_DYNAMIC_STORAGE_BIT);

		glCreateBuffers(1, &weight_buffer);
		glNamedBufferStorage(weight_buffer, max_vertices * sizeof(uint64_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glCreateBuffers(1, &index_buffer);
		glNamedBufferStorage(index_buffer, max_indices * sizeof(uint16_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glCreateBuffers(1, &texture_handles_ssbo);
		glNamedBufferStorage(texture_handles_ssbo, max_textures * sizeof(GLuint64), nullptr, GL_DYNAMIC_STORAGE_BIT);

		glCreateBuffers(1, &layer_texture_ids_ssbo);
		glNamedBufferStorage(layer_texture_ids_ssbo, max_layers * 32, nullptr, GL_DYNAMIC_STORAGE_BIT);

		glCreateBuffers(1, &layer_params_ssbo);
		glNamedBufferStorage(layer_params_ssbo, max_layers * 16, nullptr, GL_DYNAMIC_STORAGE_BIT);

		glCreateBuffers(1, &instance_ssbo);
		glCreateBuffers(1, &instance_team_color_index_ssbo);
		glCreateBuffers(1, &bone_matrices_ssbo);
		glCreateBuffers(1, &layer_colors_ssbo);
		glCreateBuffers(1, &indirect_buffer);
		glCreateBuffers(1, &draw_infos_ssbo);

		glCreateVertexArrays(1, &vao);
		glVertexArrayElementBuffer(vao, index_buffer);
	}

	Allocation reserve(const uint32_t vertices, const uint32_t indices, const uint32_t layers, const uint32_t textures) {
		std::lock_guard lock(reserve_mutex);


		if (vertex_top + vertices > max_vertices || index_top + indices > max_indices || layer_top + layers > max_layers || texture_top + textures > max_textures) {
			std::println("vertex_top {}\nindex_top {}\nlayer_top {}\ntexture_top {}", vertex_top, index_top, layer_top, texture_top);
			throw std::runtime_error("SkinnedMeshGlobals: vertex, index, layer, or texture capacity exceeded");
		}

		Allocation a {vertex_top, index_top, layer_top, texture_top};
		vertex_top += vertices;
		index_top += indices;
		layer_top += layers;
		texture_top += textures;

		return a;
	}

	void reset() {
		std::lock_guard lock(reserve_mutex);
		vertex_top = 0;
		index_top = 0;
		layer_top = 0;
		texture_top = 0;
	}

	void bind_static_ssbos() const {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, uv_snorm_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vertex_snorm_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, tangent_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, normal_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, weight_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, texture_handles_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, layer_texture_ids_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, layer_params_ssbo);
	}

	void bind_per_frame_ssbos() const {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, layer_colors_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, instance_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, bone_matrices_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, instance_team_color_index_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, draw_infos_ssbo);
	}

  private:
	std::mutex reserve_mutex;
	uint32_t vertex_top = 0;
	uint32_t index_top = 0;
	uint32_t layer_top = 0;
	uint32_t texture_top = 0;
};

export inline SkinnedMeshGlobals skinned_mesh_globals;
