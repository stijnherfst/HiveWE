export module ParticleEmitter2Simulation;

import std;
import MDX;
import <glm/glm.hpp>;

export struct EmitterPool {
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> velocities;
	std::vector<float> ages;
	size_t alive_count = 0;

	float emission_accumulator = 0.f;
	uint32_t rng_state = 0;
};

auto rand_bits(EmitterPool& p) -> uint32_t {
	uint32_t x = p.rng_state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	if (x == 0) {
		x = 1;
	}
	p.rng_state = x;
	return x;
}

auto rand01(EmitterPool& p) -> float {
	return static_cast<float>(rand_bits(p)) * (1.f / 4294967296.f);
}

auto rand_signed(EmitterPool& p) -> float {
	return rand01(p) * 2.f - 1.f;
}

export class ParticleEmitter2Simulation {
  public:
	std::vector<EmitterPool> pools;

	struct EmitterFrameParams {
		float emission_rate;
		float speed;
		float speed_variation;
		float latitude;
		float gravity;
		float width;
		float length;
		float visibility;
		glm::mat4 world_matrix;
		bool sequence_just_wrapped;
	};

	void init(const mdx::MDX& mdx) {
		pools.resize(mdx.emitters2.size());

		for (size_t i = 0; i < mdx.emitters2.size(); ++i) {
			const mdx::ParticleEmitter2& e = mdx.emitters2[i];

			float peak_rate = e.emission_rate;
			// Emmission rate can be animated so need to take the max of all tracks.
			for (const auto& t : e.KP2E.tracks) {
				peak_rate = std::max(peak_rate, t.value);
			}

			const float capacity_f = std::ceil(1.15f * peak_rate * e.life_span);
			const size_t capacity = capacity_f > 0.f ? static_cast<size_t>(capacity_f) : 0u;

			EmitterPool& pool = pools[i];
			pool.positions.assign(capacity, glm::vec3(0.f));
			pool.velocities.assign(capacity, glm::vec3(0.f));
			pool.ages.assign(capacity, 0.f);
			pool.alive_count = 0;
			pool.emission_accumulator = 0.f;
			pool.rng_state = static_cast<uint32_t>(i) * 0x9E3779B9u + 1u;
		}
	}

	void
	create_particle(const float elapsed, EmitterPool& pool, const EmitterFrameParams& params, const mdx::ParticleEmitter2& emitter) const {
		const glm::vec3 local_pos(params.width * 0.5f * rand_signed(pool), params.length * 0.5f * rand_signed(pool), 0.f);

		const bool line_emitter = (emitter.node.flags & mdx::Node::line_emitter) != 0;

		const float rotY = params.latitude * rand_signed(pool);
		const float rotZ = line_emitter ? 0.f : std::numbers::pi * rand_signed(pool);
		const float speed_j = params.speed * (1.f + params.speed_variation * rand_signed(pool));

		const glm::vec3 local_vel {
			speed_j * std::sin(rotY) * std::cos(rotZ),
			speed_j * std::sin(rotY) * std::sin(rotZ),
			speed_j * std::cos(rotY)
		};

		const size_t slot = pool.alive_count++;
		if ((emitter.node.flags & mdx::Node::model_space) != 0) {
			pool.positions[slot] = local_pos;
			pool.velocities[slot] = local_vel;
		} else {
			pool.positions[slot] = glm::vec3(params.world_matrix * glm::vec4(local_pos, 1.f));
			pool.velocities[slot] = glm::mat3(params.world_matrix) * local_vel;
		}

		// Randomize sub-frame elapsed
		pool.ages[slot] = elapsed * rand01(pool);
	}

	void
	update_emitter(const size_t emitter_index, const double delta, const mdx::ParticleEmitter2& emitter, const EmitterFrameParams& params) {
		EmitterPool& pool = pools[emitter_index];
		const size_t capacity = pool.positions.size();
		if (capacity == 0) {
			return;
		}

		// Max 0.5 to avoid particle spawn explosion on program stall
		const float dt = static_cast<float>(std::clamp(delta, 0.0, 0.5));

		if (params.visibility > 0.f) {
			if (emitter.squirt != 0 && params.sequence_just_wrapped) {
				uint32_t burst = static_cast<uint32_t>(params.emission_rate);
				while (burst-- > 0 && pool.alive_count < capacity) {
					create_particle(0.f, pool, params, emitter);
				}
			}

			pool.emission_accumulator += dt * params.emission_rate;
			while (pool.emission_accumulator >= 1.f && pool.alive_count < capacity) {
				create_particle(dt, pool, params, emitter);
				pool.emission_accumulator -= 1.f;
			}
		}

		const float az = -params.gravity;
		const float half_az_dt2 = 0.5f * az * dt * dt;

		for (size_t i = pool.alive_count; i-- > 0;) {
			pool.ages[i] += dt;

			if (pool.ages[i] >= emitter.life_span) {
				const size_t last = --pool.alive_count;
				if (i != last) {
					pool.positions[i] = pool.positions[last];
					pool.velocities[i] = pool.velocities[last];
					pool.ages[i] = pool.ages[last];
				}
				continue;
			}

			pool.positions[i] += pool.velocities[i] * dt;
			pool.positions[i].z += half_az_dt2;
			pool.velocities[i].z += az * dt;
		}
	}
};
