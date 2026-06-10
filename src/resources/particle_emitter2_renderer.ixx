export module ParticleEmitter2Renderer;

import std;
import MDX;
import ParticleEmitter2Simulation;
import SkeletalModelInstance;
import GPUTexture;
import Shader;
import ResourceManager;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glad/glad.h>;

struct ParticleVertex {
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec4 color;
};

namespace {
	constexpr float kEpsilon = 1e-6f;

	constexpr float corner_xy[4][2] = {
		{-1.f, 1.f},
		{-1.f, -1.f},
		{1.f, 1.f},
		{1.f, -1.f},
	};

	int compute_cell(const uint32_t start, const uint32_t end, const uint32_t repeat, const float t) {
		const float r = (repeat == 0) ? 1.f : static_cast<float>(repeat);
		const uint32_t initial = (end >= start) ? start : (start + 1);
		const int delta = (end >= start) ? (end - start + 1) : (end - start - 1);
		const float eff_t = (r == 1.f) ? t : std::fmod(t * r, 1.f);
		const float val = static_cast<float>(initial) + static_cast<float>(delta) * eff_t;
		return std::max(0, static_cast<int>(val));
	}
} // namespace

export class ParticleEmitter2Renderer {
  public:
	GLuint vao = 0;
	GLuint vertex_buffer = 0;
	size_t vertex_buffer_capacity = 0;
	std::shared_ptr<Shader> shader;
	std::vector<ParticleVertex> scratch;
	std::vector<std::pair<float, size_t>> scratch_sort;

	void init() {
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

		shader = resource_manager.load<Shader>({"data/shaders/particle_emitter2.vert", "data/shaders/particle_emitter2.frag"}).value();
	}

	~ParticleEmitter2Renderer() {
		glDeleteBuffers(1, &vertex_buffer);
		glDeleteVertexArrays(1, &vao);
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
		shader->use();
		glBindVertexArray(vao);
		glUniformMatrix4fv(0, 1, GL_FALSE, &projection_view[0][0]);
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);

		for (size_t i = 0; i < mdx.emitters2.size(); ++i) {
			const mdx::ParticleEmitter2& e = mdx.emitters2[i];
			const EmitterPool& pool = skeleton.particles.pools[i];
			if (pool.alive_count == 0) {
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

			ensure_capacity(scratch.size());
			glNamedBufferSubData(vertex_buffer, 0, scratch.size() * sizeof(ParticleVertex), scratch.data());

			glUniform1i(1, static_cast<int>(e.filter_mode));
			switch (e.filter_mode) {
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

			glBindTextureUnit(0, textures[e.texture_id]->id);

			glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(scratch.size()));
		}

		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
	}

  private:
	void ensure_capacity(const size_t vertex_count) {
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
		std::vector<size_t> order_indices;

		if ((e.node.flags & mdx::Node::sort_primitives_far_z) != 0) {
			scratch_sort.clear();
			scratch_sort.reserve(pool.alive_count);
			for (size_t i = 0; i < pool.alive_count; ++i) {
				glm::vec3 wp = pool.positions[i];
				if ((e.node.flags & mdx::Node::model_space) != 0) {
					wp = glm::vec3(emitter_world_matrix * glm::vec4(wp, 1.f));
				}
				scratch_sort.emplace_back(glm::dot(wp, camera_forward), i);
			}
			std::sort(scratch_sort.begin(), scratch_sort.end(), [](const auto& a, const auto& b) {
				return a.first > b.first;
			});
			order_indices.reserve(pool.alive_count);
			for (const auto& s : scratch_sort) {
				order_indices.push_back(s.second);
			}
		}

		for (size_t k = 0; k < pool.alive_count; ++k) {
			const size_t i = (e.node.flags & mdx::Node::sort_primitives_far_z) != 0 ? order_indices[k] : k;

			glm::vec3 position = pool.positions[i];
			glm::vec3 velocity = pool.velocities[i];
			if ((e.node.flags & mdx::Node::model_space) != 0) {
				position = glm::vec3(emitter_world_matrix * glm::vec4(position, 1.f));
				velocity = glm::mat3(emitter_world_matrix) * velocity;
			}

			float prev_end_time;
			float end_time;
			glm::vec3 col_a;
			glm::vec3 col_b;
			float alpha_a;
			float alpha_b;
			float scale_a;
			float scale_b;
			glm::uvec3 head_iv;
			glm::uvec3 tail_iv;
			// Either the "birth" phase or "death" phase
			if (pool.ages[i] <= e.time_middle * e.life_span) {
				prev_end_time = 0.f;
				end_time = e.time_middle * e.life_span;
				col_a = e.start_segment_color;
				col_b = e.middle_segment_color;
				alpha_a = static_cast<float>(e.segment_alphas.x) / 255.f;
				alpha_b = static_cast<float>(e.segment_alphas.y) / 255.f;
				scale_a = e.segment_scaling.x;
				scale_b = e.segment_scaling.y;
				head_iv = e.head_intervals;
				tail_iv = e.tail_intervals;
			} else {
				prev_end_time = e.time_middle * e.life_span;
				end_time = e.life_span;
				col_a = e.middle_segment_color;
				col_b = e.end_segment_color;
				alpha_a = static_cast<float>(e.segment_alphas.y) / 255.f;
				alpha_b = static_cast<float>(e.segment_alphas.z) / 255.f;
				scale_a = e.segment_scaling.y;
				scale_b = e.segment_scaling.z;
				head_iv = e.head_decay_intervals;
				tail_iv = e.tail_decay_intervals;
			}

			const float span = end_time - prev_end_time;
			const float raw = span > 0.f ? (pool.ages[i] - prev_end_time) / span : 0.f;
			const float t = raw * 0.99f + 0.005f;

			const glm::vec3 rgb = glm::mix(col_a, col_b, t);
			const float alpha = glm::mix(alpha_a, alpha_b, t);
			const float scale = glm::mix(scale_a, scale_b, t);
			const glm::vec4 color(rgb, alpha);

			const float inv_cols = 1.f / static_cast<float>(e.columns);
			const float inv_rows = 1.f / static_cast<float>(e.rows);

			if (e.head_or_tail == 0u || e.head_or_tail == 2u) {
				const int cell = compute_cell(head_iv.x, head_iv.y, head_iv.z, t);
				const int col_idx = cell % static_cast<int>(e.columns);
				const int row_idx = cell / static_cast<int>(e.columns);
				const float u0 = col_idx * inv_cols;
				const float v0u = row_idx * inv_rows;
				const float u1 = u0 + inv_cols;
				const float v1u = v0u + inv_rows;

				glm::vec3 right = camera_right;
				glm::vec3 up = camera_up;

				// XYQuad particles lie flat in the XY plane and orient along their horizontal velocity
				if ((e.node.flags & mdx::Node::xy_quad) != 0) {
					const glm::vec3 vel_xy(velocity.x, velocity.y, 0.f);
					const float mag2 = vel_xy.x * vel_xy.x + vel_xy.y * vel_xy.y;
					if (mag2 > kEpsilon) {
						up = glm::normalize(vel_xy);
						right = glm::vec3(velocity.y, -velocity.x, 0.f) / std::sqrt(mag2);
					} else {
						// No horizontal velocity to derive a facing from.
						right = glm::vec3(1.f, 0.f, 0.f);
						up = glm::vec3(0.f, 1.f, 0.f);
					}
				}

				const glm::vec3 ax = right * scale;
				const glm::vec3 ay = up * scale;

				const glm::vec3 c0 = position + ax * corner_xy[0][0] + ay * corner_xy[0][1];
				const glm::vec3 c1 = position + ax * corner_xy[1][0] + ay * corner_xy[1][1];
				const glm::vec3 c2 = position + ax * corner_xy[2][0] + ay * corner_xy[2][1];
				const glm::vec3 c3 = position + ax * corner_xy[3][0] + ay * corner_xy[3][1];

				// Reference winding: tri 1 = (0,1,2), tri 2 = (2,1,3)
				// For head: v0=TL, v1=BL, v2=TR, v3=BR, uvs (u0,v0u),(u0,v1u),(u1,v0u),(u1,v1u)
				scratch.push_back({c0, glm::vec2(u0, v0u), color});
				scratch.push_back({c1, glm::vec2(u0, v1u), color});
				scratch.push_back({c2, glm::vec2(u1, v0u), color});
				scratch.push_back({c2, glm::vec2(u1, v0u), color});
				scratch.push_back({c1, glm::vec2(u0, v1u), color});
				scratch.push_back({c3, glm::vec2(u1, v1u), color});
			}

			if (e.head_or_tail == 1u || e.head_or_tail == 2u) {
				const glm::vec3 neg_vel = -velocity * e.tail_length;
				const float neg_vel_mag2 = glm::dot(neg_vel, neg_vel);

				if (neg_vel_mag2 <= kEpsilon) {
					continue;
				}

				const glm::vec3 tail_end = position + neg_vel;
				const glm::vec3 tail_dir = glm::normalize(neg_vel);
				glm::vec3 perp = glm::cross(tail_dir, camera_forward);
				if (glm::dot(perp, perp) < kEpsilon) {
					const glm::vec3 alt_up = (std::abs(tail_dir.z) > 0.999f) ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(0.f, 0.f, 1.f);
					perp = glm::cross(tail_dir, alt_up);
					if (glm::dot(perp, perp) <= kEpsilon) {
						continue;
					}
				}

				perp = glm::normalize(perp);

				const int cell = compute_cell(tail_iv.x, tail_iv.y, tail_iv.z, t);
				const int col_idx = cell % static_cast<int>(e.columns);
				const int row_idx = cell / static_cast<int>(e.columns);
				const float u0 = col_idx * inv_cols;
				const float v0u = row_idx * inv_rows;
				const float u1 = u0 + inv_cols;
				const float v1u = v0u + inv_rows;

				const glm::vec3 w = perp * scale;
				const glm::vec3 hl = position - w;
				const glm::vec3 hr = position + w;
				const glm::vec3 tl = tail_end - w;
				const glm::vec3 tr = tail_end + w;

				// Reference tail emission: hl,hr,tl, tr,tl,hr with UVs hl=(u0,v0),hr=(u0,v1),tl=(u1,v0),tr=(u1,v1)
				scratch.push_back({hl, glm::vec2(u0, v0u), color});
				scratch.push_back({hr, glm::vec2(u0, v1u), color});
				scratch.push_back({tl, glm::vec2(u1, v0u), color});
				scratch.push_back({tr, glm::vec2(u1, v1u), color});
				scratch.push_back({tl, glm::vec2(u1, v0u), color});
				scratch.push_back({hr, glm::vec2(u0, v1u), color});
			}
		}
	}
};
