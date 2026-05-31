export module ParticleEmitter2Renderer;

import std;
import MDX;
import ParticleEmitter2Simulation;
import SkeletalModelInstance;
import GPUTexture;
import Shader;
import ResourceManager;
import <glm/glm.hpp>;
import <glad/glad.h>;

struct ParticleVertex {
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec4 color;
};

namespace {
	constexpr float kEpsilon = 1e-6f;

	constexpr float corner_xy[4][2] = {
		{-1.f,  1.f},
		{-1.f, -1.f},
		{ 1.f,  1.f},
		{ 1.f, -1.f},
	};

	inline float key_t(const float age, const float prev_end_time, const float end_time) {
		const float span = end_time - prev_end_time;
		const float raw = span > 0.f ? (age - prev_end_time) / span : 0.f;
		return std::clamp(raw, 0.f, 1.f) * 0.99f + 0.005f;
	}

	inline int compute_cell(int start, int end, int repeat, float t) {
		const float r = (repeat < 1) ? 1.f : static_cast<float>(repeat);
		const int initial = (end >= start) ? start : (start + 1);
		const int delta   = (end >= start) ? (end - start + 1) : (end - start - 1);
		const float eff_t = (r == 1.f) ? t : std::fmod(t * r, 1.f);
		const float val = static_cast<float>(initial) + static_cast<float>(delta) * eff_t;
		return std::max(0, static_cast<int>(val));
	}
}

export class ParticleEmitter2Renderer {
  public:
	GLuint vao = 0;
	GLuint vertex_buffer = 0;
	size_t vertex_buffer_capacity = 0;
	std::shared_ptr<Shader> shader;
	std::vector<ParticleVertex> scratch;
	std::vector<std::pair<float, size_t>> scratch_sort;

	void init() {
		if (vao != 0) {
			return;
		}

		glCreateVertexArrays(1, &vao);
		glCreateBuffers(1, &vertex_buffer);

		vertex_buffer_capacity = 1024 * 12;
		glNamedBufferData(vertex_buffer, vertex_buffer_capacity * sizeof(ParticleVertex), nullptr, GL_DYNAMIC_DRAW);

		glEnableVertexArrayAttrib(vao, 0);
		glEnableVertexArrayAttrib(vao, 1);
		glEnableVertexArrayAttrib(vao, 2);

		glVertexArrayVertexBuffer(vao, 0, vertex_buffer, 0, sizeof(ParticleVertex));

		glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(ParticleVertex, position));
		glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, offsetof(ParticleVertex, uv));
		glVertexArrayAttribFormat(vao, 2, 4, GL_FLOAT, GL_FALSE, offsetof(ParticleVertex, color));

		glVertexArrayAttribBinding(vao, 0, 0);
		glVertexArrayAttribBinding(vao, 1, 0);
		glVertexArrayAttribBinding(vao, 2, 0);

		shader = resource_manager.load<Shader>({ "data/shaders/particle_emitter2.vert", "data/shaders/particle_emitter2.frag" }).value();
	}

	~ParticleEmitter2Renderer() {
		if (vertex_buffer != 0) {
			glDeleteBuffers(1, &vertex_buffer);
		}
		if (vao != 0) {
			glDeleteVertexArrays(1, &vao);
		}
	}

	void render(
		const mdx::MDX& mdx,
		const std::vector<std::shared_ptr<GPUTexture>>& textures,
		const SkeletalModelInstance& skeleton,
		const glm::mat4& projection_view,
		const glm::vec3& camera_right,
		const glm::vec3& camera_up,
		const glm::vec3& camera_forward
	) {
		if (vao == 0) {
			return;
		}

		bool any_drawn = false;

		for (size_t i = 0; i < mdx.emitters2.size(); ++i) {
			const mdx::ParticleEmitter2& e = mdx.emitters2[i];
			if (i >= skeleton.particles.pools.size()) {
				continue;
			}
			const EmitterPool& pool = skeleton.particles.pools[i];
			if (pool.alive_count == 0) {
				continue;
			}
			if (e.texture_id >= textures.size()) {
				continue;
			}

			// Match the simulation path: append the pivot translation since
			// world_matrices[node.id] is in skinning convention and omits it at rest.
			const glm::mat4 emitter_world_matrix =
				skeleton.world_matrices[e.node.id] * glm::translate(glm::mat4(1.f), mdx.pivots[e.node.id]);

			scratch.clear();
			build_emitter_vertices(e, pool, emitter_world_matrix, camera_right, camera_up, camera_forward);
			if (scratch.empty()) {
				continue;
			}

			if (!any_drawn) {
				shader->use();
				glBindVertexArray(vao);
				glUniformMatrix4fv(0, 1, GL_FALSE, &projection_view[0][0]);
				glEnable(GL_BLEND);
				glDepthMask(GL_FALSE);
				glDisable(GL_CULL_FACE);
				any_drawn = true;
			}

			ensure_capacity(scratch.size());
			glNamedBufferSubData(vertex_buffer, 0, scratch.size() * sizeof(ParticleVertex), scratch.data());

			glUniform1i(1, static_cast<int>(e.filter_mode));
			set_blend_mode(e.filter_mode);

			glBindTextureUnit(0, textures[e.texture_id]->id);

			glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(scratch.size()));
		}

		if (any_drawn) {
			glDepthMask(GL_TRUE);
			glEnable(GL_CULL_FACE);
		}
	}

  private:
	void ensure_capacity(size_t vertex_count) {
		if (vertex_count <= vertex_buffer_capacity) {
			return;
		}
		size_t new_capacity = vertex_buffer_capacity == 0 ? 1024u : vertex_buffer_capacity;
		while (new_capacity < vertex_count) {
			new_capacity *= 2;
		}
		glNamedBufferData(vertex_buffer, new_capacity * sizeof(ParticleVertex), nullptr, GL_DYNAMIC_DRAW);
		vertex_buffer_capacity = new_capacity;
	}

	void build_emitter_vertices(
		const mdx::ParticleEmitter2& e,
		const EmitterPool& pool,
		const glm::mat4& emitter_world_matrix,
		const glm::vec3& camera_right,
		const glm::vec3& camera_up,
		const glm::vec3& camera_forward
	) {
		const float life_span = e.life_span;
		const float middle_time = e.time_middle * life_span;

		const bool model_space = (e.node.flags & mdx::Node::model_space) != 0;
		const bool xy_quad     = (e.node.flags & mdx::Node::xy_quad) != 0;
		const bool sort_z      = (e.node.flags & mdx::Node::sort_primitives_far_z) != 0;

		const bool has_head = (e.head_or_tail == 0u || e.head_or_tail == 2u);
		const bool has_tail = (e.head_or_tail == 1u || e.head_or_tail == 2u);

		const uint32_t rows = e.rows == 0u ? 1u : e.rows;
		const uint32_t cols = e.columns == 0u ? 1u : e.columns;
		const float inv_cols = 1.f / static_cast<float>(cols);
		const float inv_rows = 1.f / static_cast<float>(rows);

		const float tail_length = e.tail_length;

		std::vector<size_t> order_indices;
		const size_t alive = pool.alive_count;

		if (sort_z) {
			scratch_sort.clear();
			scratch_sort.reserve(alive);
			for (size_t i = 0; i < alive; ++i) {
				glm::vec3 wp = pool.positions[i];
				if (model_space) {
					wp = glm::vec3(emitter_world_matrix * glm::vec4(wp, 1.f));
				}
				scratch_sort.emplace_back(glm::dot(wp, camera_forward), i);
			}
			std::sort(scratch_sort.begin(), scratch_sort.end(),
				[](const auto& a, const auto& b) { return a.first > b.first; });
			order_indices.reserve(alive);
			for (const auto& s : scratch_sort) {
				order_indices.push_back(s.second);
			}
		}

		auto emit_quad = [&](const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3,
		                     float u0, float v0u, float u1, float v1u,
		                     const glm::vec4& col) {
			// Reference winding: tri 1 = (0,1,2), tri 2 = (2,1,3)
			// For head: v0=TL, v1=BL, v2=TR, v3=BR, uvs (u0,v0u),(u0,v1u),(u1,v0u),(u1,v1u)
			scratch.push_back({v0, glm::vec2(u0, v0u), col});
			scratch.push_back({v1, glm::vec2(u0, v1u), col});
			scratch.push_back({v2, glm::vec2(u1, v0u), col});
			scratch.push_back({v2, glm::vec2(u1, v0u), col});
			scratch.push_back({v1, glm::vec2(u0, v1u), col});
			scratch.push_back({v3, glm::vec2(u1, v1u), col});
		};

		for (size_t k = 0; k < alive; ++k) {
			const size_t i = sort_z ? order_indices[k] : k;

			glm::vec3 wp = pool.positions[i];
			glm::vec3 wv = pool.velocities[i];
			if (model_space) {
				wp = glm::vec3(emitter_world_matrix * glm::vec4(wp, 1.f));
				wv = glm::mat3(emitter_world_matrix) * wv;
			}

			const float age = pool.ages[i];
			const uint32_t kf = pool.key_frames[i] >= 2u ? 1u : pool.key_frames[i];

			const float prev_end = (kf == 0u) ? 0.f : middle_time;
			const float end_t    = (kf == 0u) ? middle_time : life_span;
			const float t = key_t(age, prev_end, end_t);

			glm::vec3 col_a;
			glm::vec3 col_b;
			float alpha_a;
			float alpha_b;
			float scale_a;
			float scale_b;
			glm::uvec3 head_iv;
			glm::uvec3 tail_iv;
			if (kf == 0u) {
				col_a = e.start_segment_color;
				col_b = e.middle_segment_color;
				alpha_a = static_cast<float>(e.segment_alphas.x) / 255.f;
				alpha_b = static_cast<float>(e.segment_alphas.y) / 255.f;
				scale_a = e.segment_scaling.x;
				scale_b = e.segment_scaling.y;
				head_iv = e.head_intervals;
				tail_iv = e.tail_intervals;
			} else {
				col_a = e.middle_segment_color;
				col_b = e.end_segment_color;
				alpha_a = static_cast<float>(e.segment_alphas.y) / 255.f;
				alpha_b = static_cast<float>(e.segment_alphas.z) / 255.f;
				scale_a = e.segment_scaling.y;
				scale_b = e.segment_scaling.z;
				head_iv = e.head_decay_intervals;
				tail_iv = e.tail_decay_intervals;
			}

			const glm::vec3 rgb = col_a + (col_b - col_a) * t;
			const float a = alpha_a + (alpha_b - alpha_a) * t;
			const float scale = scale_a + (scale_b - scale_a) * t;
			const glm::vec4 vcol(rgb, a);

			glm::vec3 right = camera_right;
			glm::vec3 up    = camera_up;

			if (xy_quad) {
				const glm::vec3 vel_xy(wv.x, wv.y, 0.f);
				const glm::vec3 perp_xy(wv.y, -wv.x, 0.f);
				const float mag2 = vel_xy.x * vel_xy.x + vel_xy.y * vel_xy.y;
				if (mag2 > kEpsilon) {
					right = glm::normalize(perp_xy);
					up    = glm::normalize(vel_xy);
				}
			}

			if (has_head) {
				const int cell = compute_cell(static_cast<int>(head_iv.x), static_cast<int>(head_iv.y), static_cast<int>(head_iv.z), t);
				const int col_idx = cell % static_cast<int>(cols);
				const int row_idx = cell / static_cast<int>(cols);
				const float u0 = col_idx * inv_cols;
				const float v0u = row_idx * inv_rows;
				const float u1 = u0 + inv_cols;
				const float v1u = v0u + inv_rows;

				const glm::vec3 ax = right * scale;
				const glm::vec3 ay = up    * scale;

				const glm::vec3 c0 = wp + ax * corner_xy[0][0] + ay * corner_xy[0][1];
				const glm::vec3 c1 = wp + ax * corner_xy[1][0] + ay * corner_xy[1][1];
				const glm::vec3 c2 = wp + ax * corner_xy[2][0] + ay * corner_xy[2][1];
				const glm::vec3 c3 = wp + ax * corner_xy[3][0] + ay * corner_xy[3][1];

				emit_quad(c0, c1, c2, c3, u0, v0u, u1, v1u, vcol);
			}

			if (has_tail) {
				const glm::vec3 neg_vel = -wv * tail_length;
				const float neg_vel_mag2 = glm::dot(neg_vel, neg_vel);
				if (neg_vel_mag2 > kEpsilon) {
					const glm::vec3 tail_end = wp + neg_vel;
					const glm::vec3 tail_dir = glm::normalize(neg_vel);
					glm::vec3 perp = glm::cross(tail_dir, camera_forward);
					float perp_mag2 = glm::dot(perp, perp);
					if (perp_mag2 < kEpsilon) {
						const glm::vec3 alt_up = (std::abs(tail_dir.z) > 0.999f) ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(0.f, 0.f, 1.f);
						perp = glm::cross(tail_dir, alt_up);
						perp_mag2 = glm::dot(perp, perp);
					}
					if (perp_mag2 > kEpsilon) {
						perp = glm::normalize(perp);

						const int cell = compute_cell(static_cast<int>(tail_iv.x), static_cast<int>(tail_iv.y), static_cast<int>(tail_iv.z), t);
						const int col_idx = cell % static_cast<int>(cols);
						const int row_idx = cell / static_cast<int>(cols);
						const float u0 = col_idx * inv_cols;
						const float v0u = row_idx * inv_rows;
						const float u1 = u0 + inv_cols;
						const float v1u = v0u + inv_rows;

						const glm::vec3 w = perp * scale;
						const glm::vec3 hl = wp - w;
						const glm::vec3 hr = wp + w;
						const glm::vec3 tl = tail_end - w;
						const glm::vec3 tr = tail_end + w;

						// Reference tail emission: hl,hr,tl, tr,tl,hr with UVs hl=(u0,v0),hr=(u0,v1),tl=(u1,v0),tr=(u1,v1)
						scratch.push_back({hl, glm::vec2(u0, v0u), vcol});
						scratch.push_back({hr, glm::vec2(u0, v1u), vcol});
						scratch.push_back({tl, glm::vec2(u1, v0u), vcol});
						scratch.push_back({tr, glm::vec2(u1, v1u), vcol});
						scratch.push_back({tl, glm::vec2(u1, v0u), vcol});
						scratch.push_back({hr, glm::vec2(u0, v1u), vcol});
					}
				}
			}
		}
	}

	static void set_blend_mode(uint32_t filter_mode) {
		switch (filter_mode) {
			case 0: // Blend
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case 1: // Additive
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				break;
			case 2: // Modulate
				glBlendFunc(GL_ZERO, GL_SRC_COLOR);
				break;
			case 3: // Modulate2x
				glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
				break;
			case 4: // AlphaKey (alpha-discard handled in shader)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			default:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
		}
	}
};
