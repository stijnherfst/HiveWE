// Particle emitter 2 values that MDX::is_valid() deliberately lets through.
//
// The game accepts these models, so we must render them rather than refuse them. Reading the engine
// showed what each one does there; these tests pin down that fix_up() repairs what needs repairing
// and that the simulation survives the rest.

#include <algorithm>
#include <doctest/doctest.h>

import MDX;
import ParticleEmitter2Simulation;
import <glm/glm.hpp>;

namespace {
	/// An emitter that emits steadily, so a test only has to change the one field it cares about.
	mdx::ParticleEmitter2 base_emitter() {
		mdx::ParticleEmitter2 emitter {};
		emitter.life_span = 1.f;
		emitter.emission_rate = 10.f;
		emitter.speed = 100.f;
		emitter.time_middle = 0.5f;
		emitter.rows = 1;
		emitter.columns = 1;
		emitter.tail_length = 1.f;
		emitter.head_or_tail = 0;
		return emitter;
	}

	/// A one-emitter model. fix_up() adds a bone of its own if none exists, but the texture the
	/// emitter points at has to be there or is_valid() rejects the model for an unrelated reason.
	mdx::MDX model_with(const mdx::ParticleEmitter2& emitter) {
		mdx::MDX model {};
		model.textures.push_back(mdx::Texture { .file_name = "Textures/white.blp" });
		model.emitters2.push_back(emitter);
		model.emitters2[0].node.id = 0;
		model.emitters2[0].node.parent_id = -1;
		return model;
	}

	ParticleEmitter2Simulation::EmitterFrameParams frame_params(const mdx::ParticleEmitter2& emitter) {
		ParticleEmitter2Simulation::EmitterFrameParams params {};
		params.emission_rate = emitter.emission_rate;
		params.speed = emitter.speed;
		params.visibility = 1.f;
		params.world_matrix = glm::mat4(1.f);
		return params;
	}

	/// Runs a model with one emitter for a while and returns how many particles are alive at the end.
	size_t simulate(const mdx::ParticleEmitter2& emitter, const bool wrap_sequence = false) {
		mdx::MDX model {};
		model.emitters2.push_back(emitter);

		ParticleEmitter2Simulation simulation;
		simulation.init(model);

		auto params = frame_params(emitter);
		params.sequence_just_wrapped = wrap_sequence;
		for (int frame = 0; frame < 120; ++frame) {
			simulation.update_emitter(0, 1.0 / 60.0, emitter, params);
			params.sequence_just_wrapped = false;
		}
		return simulation.pools[0].alive_count;
	}
}

TEST_CASE("fix_up: an emitter survives a pivot buffer that is too small") {
	// calculate_extents() bounds each emitter by pivots[node.id], so the pivot buffer has to be grown
	// before it runs rather than after. A model with an emitter and no pivots used to read past the end.
	mdx::MDX model = model_with(base_emitter());
	REQUIRE(model.pivots.empty());

	model.fix_up();

	CHECK(model.pivots.size() >= model.emitters2.size());
	for (const auto& emitter : model.emitters2) {
		CHECK(static_cast<size_t>(emitter.node.id) < model.pivots.size());
	}
}

TEST_CASE("fix_up: a valid texture grid is left alone") {
	for (const auto [rows, columns] : { std::pair<uint32_t, uint32_t> { 1, 1 }, { 2, 4 }, { 8, 8 }, { 16, 1 } }) {
		CAPTURE(rows);
		CAPTURE(columns);
		mdx::ParticleEmitter2 emitter = base_emitter();
		emitter.rows = rows;
		emitter.columns = columns;

		mdx::MDX model = model_with(emitter);
		model.fix_up();

		CHECK(model.emitters2[0].rows == rows);
		CHECK(model.emitters2[0].columns == columns);
	}
}

TEST_CASE("fix_up: a texture grid the game rejects collapses to 1x1") {
	// The game validates both dimensions in one call and stores neither if either is bad, so a good
	// row count does not rescue a bad column count. Zero must not survive: the renderer divides by
	// the column count.
	for (const auto [rows, columns] : { std::pair<uint32_t, uint32_t> { 0, 4 }, { 4, 0 }, { 0, 0 }, { 3, 2 }, { 7, 1 }, { 1, 6 } }) {
		CAPTURE(rows);
		CAPTURE(columns);
		mdx::ParticleEmitter2 emitter = base_emitter();
		emitter.rows = rows;
		emitter.columns = columns;

		mdx::MDX model = model_with(emitter);
		model.fix_up();

		CHECK(model.emitters2[0].rows == 1);
		CHECK(model.emitters2[0].columns == 1);
	}
}

TEST_CASE("Validator: a bad texture grid is still reported before fix_up repairs it") {
	// fix_up() makes the model renderable; validate() still has to tell the author the game will not
	// show what they authored.
	mdx::ParticleEmitter2 emitter = base_emitter();
	emitter.rows = 0;
	emitter.columns = 0;

	mdx::MDX model = model_with(emitter);
	const auto messages = model.validate();
	const bool reported = std::ranges::any_of(messages, [](const mdx::ValidationMessage& message) {
		return message.severity == mdx::ValidationSeverity::severe && message.message.contains("texture grid");
	});
	CHECK(reported);

	// And it is fatal to nothing: the model still renders.
	CHECK(model.is_valid());
}

TEST_CASE("Emitter simulation: a negative lifespan emits nothing") {
	// The game destroys a particle once its age reaches the lifespan, so a negative one kills every
	// particle on the frame it is born. We never allocate a pool for it at all.
	mdx::ParticleEmitter2 emitter = base_emitter();
	emitter.life_span = -1.5f;

	mdx::MDX model {};
	model.emitters2.push_back(emitter);
	ParticleEmitter2Simulation simulation;
	simulation.init(model);

	CHECK(simulation.pools[0].positions.empty());
	CHECK(simulate(emitter) == 0);
}

TEST_CASE("Emitter simulation: a zero lifespan emits nothing") {
	// How the shipped models disable an emitter, so it must behave exactly like the negative case.
	mdx::ParticleEmitter2 emitter = base_emitter();
	emitter.life_span = 0.f;
	CHECK(simulate(emitter) == 0);
}

TEST_CASE("Emitter simulation: a time middle outside [0, 1] still runs") {
	// Only used to place the middle colour key, at time_middle * life_span. Out of range it lands
	// beyond the particle's life and the middle stage never shows, but nothing divides by it.
	mdx::ParticleEmitter2 above = base_emitter();
	above.time_middle = 3.f;
	CHECK(simulate(above) > 0);

	mdx::ParticleEmitter2 below = base_emitter();
	below.time_middle = -1.f;
	CHECK(simulate(below) > 0);
}

TEST_CASE("Emitter simulation: a negative tail length still runs") {
	// Used raw as a scale on velocity, so a negative one mirrors the tail. 33 community models do it.
	mdx::ParticleEmitter2 emitter = base_emitter();
	emitter.head_or_tail = 1;
	emitter.tail_length = -5.f;
	CHECK(simulate(emitter) > 0);
}

TEST_CASE("Emitter simulation: squirt above one bursts like squirt of one") {
	// The 2.1 engine has no squirt concept at all, and we treat any non-zero value as a burst.
	mdx::ParticleEmitter2 emitter = base_emitter();
	emitter.squirt = 7;
	CHECK(simulate(emitter, true) > 0);
}

TEST_CASE("Emitter simulation: a negative emission rate does not burst") {
	// The rate reaching the simulation is interpolated from KP2E, so it can be negative on a frame
	// even when the static value is not. Converting that to an unsigned burst count would be undefined.
	mdx::ParticleEmitter2 emitter = base_emitter();
	emitter.squirt = 1;

	mdx::MDX model {};
	model.emitters2.push_back(emitter);
	ParticleEmitter2Simulation simulation;
	simulation.init(model);

	auto params = frame_params(emitter);
	params.emission_rate = -50.f;
	params.sequence_just_wrapped = true;
	simulation.update_emitter(0, 1.0 / 60.0, emitter, params);

	CHECK(simulation.pools[0].alive_count == 0);
}

TEST_CASE("Emitter simulation: an unknown head or tail value still runs") {
	// The game switches on 0, 1 and 2 and sets no style for anything else; we draw no quad for it.
	mdx::ParticleEmitter2 emitter = base_emitter();
	emitter.head_or_tail = 5;
	CHECK(simulate(emitter) > 0);
}

TEST_CASE("Emitter simulation: a zero texture grid still runs") {
	// The grid does not reach the simulation at all, only the renderer, but a model carrying one must
	// still run rather than be refused.
	mdx::ParticleEmitter2 emitter = base_emitter();
	emitter.rows = 0;
	emitter.columns = 0;
	CHECK(simulate(emitter) > 0);
}
