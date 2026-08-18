// Node ids in MDX::fix_up().
//
// A node id is whatever the file said it was, and it indexes the pivots, the renderer's world matrix
// array and the geosets' matrix indices. fix_up() used to renumber the nodes into their walk order
// without rewriting the other two, which silently mis-skinned every model it touched. It now leaves
// the ids alone and is_valid() rejects the models that would have needed renumbering, so what these
// cases pin is that fix_up() survives a bad id rather than repairing it.

#include <algorithm>
#include <doctest/doctest.h>

import MDX;
import <glm/glm.hpp>;

namespace {
	/// A bone chain, one bone per (id, parent_id) pair, with a pivot per bone marking where it began.
	mdx::MDX model_with_bones(const std::vector<std::pair<int, int>>& ids) {
		mdx::MDX model {};
		for (const auto& [id, parent_id] : ids) {
			auto& bone = model.bones.emplace_back();
			bone.node.id = id;
			bone.node.parent_id = parent_id;
		}
		for (size_t i = 0; i < ids.size(); ++i) {
			model.pivots.push_back(glm::vec3(static_cast<float>(i), 0.f, 0.f));
		}
		return model;
	}

	/// Attach a minimal renderable material (one texture, one layer using it) so a geoset-bearing model
	/// satisfies is_valid()'s material checks. These cases isolate skinning, not materials, but a geoset
	/// with no material would crash the renderer and so is now rejected.
	void give_material(mdx::MDX& model) {
		model.textures.emplace_back();
		auto& material = model.materials.emplace_back();
		material.layers.emplace_back().textures.emplace_back();
	}

	bool has_message(const std::vector<mdx::ValidationMessage>& messages, const mdx::ValidationSeverity severity,
		const std::string_view fragment) {
		return std::ranges::any_of(messages, [&](const mdx::ValidationMessage& message) {
			return message.severity == severity && message.message.contains(fragment);
		});
	}
}

TEST_CASE("Validator: a negative node id is an error and fails is_valid") {
	mdx::MDX model = model_with_bones({ { 0, -1 }, { -1, 0 } });

	CHECK(!model.is_valid());

	const auto messages = model.validate();
	CHECK(has_message(messages, mdx::ValidationSeverity::error, "has invalid ID -1"));

	// A negative id converts to a huge unsigned value, so it used to report itself as being past the
	// node count as well.
	CHECK(!has_message(messages, mdx::ValidationSeverity::error, "higher than node count"));
}

TEST_CASE("fix_up: geoset matrix indices still point at the bones they did") {
	mdx::MDX model = model_with_bones({ { 0, -1 }, { 1, 0 }, { 2, 0 } });
	auto& geoset = model.geosets.emplace_back();
	give_material(model);
	geoset.vertices.assign(3, glm::vec3(0.f));
	geoset.vertex_groups.assign(3, 0);
	geoset.matrix_groups = { 1 };
	geoset.matrix_indices = { 2 };

	model.fix_up();

	REQUIRE(model.geosets.size() == 1);
	REQUIRE(model.geosets[0].matrix_indices.size() == 1);
	CHECK(model.geosets[0].matrix_indices[0] == 2);
	CHECK(model.bones[2].node.id == 2);
}

TEST_CASE("fix_up: a parent no node owns becomes a root") {
	// Attaching the node to whichever node happens to sit at that index would be a guess; the game
	// has no node there either.
	mdx::MDX model = model_with_bones({ { 0, -1 }, { 1, 77 } });

	model.fix_up();

	CHECK(model.bones[1].node.parent_id == -1);
}

TEST_CASE("Validator: repeated node ids fail is_valid") {
	mdx::MDX model = model_with_bones({ { 5, -1 }, { 5, 5 }, { 5, 5 } });

	CHECK(!model.is_valid());

	const auto messages = model.validate();
	CHECK(has_message(messages, mdx::ValidationSeverity::error, "duplicated ID 5"));
}

TEST_CASE("fix_up: a model with no bones gets one at object id 0") {
	// Skinning resolves an object id against a palette holding only the bones, so a synthesised bone
	// has to take the lowest id and push everything else up rather than take the next free one.
	mdx::MDX model {};
	auto& helper = model.help_bones.emplace_back();
	helper.id = 0;
	helper.parent_id = -1;
	auto& emitter = model.emitters2.emplace_back();
	emitter.node.id = 1;
	emitter.node.parent_id = 0;
	model.textures.emplace_back(); // An emitter with no texture to point at is invalid on its own.
	model.pivots = { glm::vec3(7.f, 0.f, 0.f), glm::vec3(8.f, 0.f, 0.f) };

	model.fix_up();

	REQUIRE(model.bones.size() == 1);
	CHECK(model.bones[0].node.id == 0);
	CHECK(model.bones[0].node.parent_id == -1);
	CHECK(model.help_bones[0].id == 1);
	CHECK(model.help_bones[0].parent_id == -1);
	CHECK(model.emitters2[0].node.id == 2);
	CHECK(model.emitters2[0].node.parent_id == 1);

	// The pivots move up with the nodes that own them.
	REQUIRE(model.pivots.size() == 3);
	CHECK(model.pivots[1].x == doctest::Approx(7.f));
	CHECK(model.pivots[2].x == doctest::Approx(8.f));

	CHECK(model.is_valid());
}

TEST_CASE("fix_up: a model with no nodes at all keeps its skinning pointed at object id 0") {
	// merge_with() builds exactly this: geosets skinned to object id 0 with no node anywhere yet, so
	// the bone fix_up() synthesises has to be the thing they already point at. Shifting them up to
	// make room, which is right when there are nodes to move, would leave them pointing at nothing.
	mdx::MDX model {};
	auto& geoset = model.geosets.emplace_back();
	give_material(model);
	geoset.vertices.assign(3, glm::vec3(0.f));
	geoset.vertex_groups.assign(3, 0);
	geoset.matrix_groups = { 1 };
	geoset.matrix_indices = { 0 };
	geoset.skin.assign(3 * 8, 0);
	for (size_t i = 0; i < geoset.skin.size(); i += 8) {
		geoset.skin[i + 4] = 255;
	}

	model.fix_up();

	REQUIRE(model.bones.size() == 1);
	CHECK(model.bones[0].node.id == 0);
	CHECK(model.geosets[0].matrix_indices[0] == 0);
	CHECK(model.geosets[0].skin[0] == 0);
	CHECK(model.is_valid());
}

TEST_CASE("fix_up: a boneless model binds its skinning to the bone it gains") {
	// Warcraft 3 draws a boneless model's geosets from the model transform without ever reading the
	// matrix indices, so an index naming an object that does not exist still renders. It has to end
	// up on the identity bone rather than kept or shifted, or a model the game draws comes out
	// mis-skinned here.
	mdx::MDX model {};
	auto& geoset = model.geosets.emplace_back();
	give_material(model);
	geoset.vertices.assign(3, glm::vec3(0.f));
	geoset.vertex_groups.assign(3, 0);
	geoset.matrix_groups = { 1 };
	geoset.matrix_indices = { 200 };

	model.fix_up();

	REQUIRE(model.bones.size() == 1);
	CHECK(model.bones[0].node.id == 0);
	CHECK(model.geosets[0].matrix_indices[0] == 0);
	CHECK(model.is_valid());
}

TEST_CASE("fix_up: a boneless model with other nodes does not skin to them") {
	// 267 models in the dev build are CORN-only. Shifting the matrix indices up with the nodes would
	// bind the geoset to a particle emitter that moves, where the game draws it static.
	mdx::MDX model {};
	auto& emitter = model.corn_emitters.emplace_back();
	emitter.node.id = 0;
	emitter.node.parent_id = -1;
	auto& geoset = model.geosets.emplace_back();
	give_material(model);
	geoset.vertices.assign(3, glm::vec3(0.f));
	geoset.vertex_groups.assign(3, 0);
	geoset.matrix_groups = { 1 };
	geoset.matrix_indices = { 0 };

	model.fix_up();

	REQUIRE(model.bones.size() == 1);
	CHECK(model.bones[0].node.id == 0);
	CHECK(model.corn_emitters[0].node.id == 1);
	CHECK(model.geosets[0].matrix_indices[0] == 0);
	CHECK(model.is_valid());
}

TEST_CASE("Validator: a bone numbered above the bone count is an error") {
	// The renderer uploads only the first bones.size() world matrices and Warcraft 3 builds its
	// palette the same way, so a helper holding an object id below a bone's drops that bone's
	// vertices. One model in the 35471 on Hiveworkshop does this: Goblin Factory.
	mdx::MDX model {};
	auto& helper = model.help_bones.emplace_back();
	helper.id = 0;
	helper.parent_id = -1;
	auto& bone = model.bones.emplace_back();
	bone.node.name = "Arm";
	bone.node.id = 1;
	bone.node.parent_id = 0;
	model.pivots.assign(2, glm::vec3(0.f));

	CHECK(!model.is_valid());
	CHECK(has_message(model.validate(), mdx::ValidationSeverity::error, "will not render"));
}

TEST_CASE("Validator: bones numbered below every other node are accepted") {
	mdx::MDX model {};
	auto& bone = model.bones.emplace_back();
	bone.node.id = 0;
	bone.node.parent_id = -1;
	auto& helper = model.help_bones.emplace_back();
	helper.id = 1;
	helper.parent_id = 0;
	model.pivots.assign(2, glm::vec3(0.f));

	CHECK(model.is_valid());
	CHECK(!has_message(model.validate(), mdx::ValidationSeverity::error, "will not render"));
}

TEST_CASE("Validator: a skinning index past the node count fails is_valid") {
	// Skeleton::build_sample_frames does world_matrices[indices[j]] with no bound of its own, and the
	// shaders index the uploaded palette the same way, so this has to be caught before anything
	// renders rather than clamped afterwards.
	mdx::MDX model = model_with_bones({ { 0, -1 }, { 1, 0 } });
	auto& geoset = model.geosets.emplace_back();
	give_material(model);
	geoset.vertices.assign(3, glm::vec3(0.f));
	geoset.vertex_groups.assign(3, 0);
	geoset.matrix_groups = { 1 };

	SUBCASE("SD matrix index inside the node count") {
		geoset.matrix_indices = { 1 };
		CHECK(model.is_valid());
	}

	SUBCASE("SD matrix index past the node count") {
		geoset.matrix_indices = { 7 };
		CHECK(!model.is_valid());
	}

	SUBCASE("HD skin index past the node count") {
		geoset.matrix_indices = { 0 };
		geoset.skin.assign(3 * 8, 0);
		for (size_t i = 0; i < geoset.skin.size(); i += 8) {
			geoset.skin[i] = 9;
			geoset.skin[i + 4] = 255;
		}
		CHECK(!model.is_valid());
	}

	SUBCASE("HD skin index past the node count but carrying no weight") {
		// The renderer skips a zero-weight slot, so its index is never dereferenced.
		geoset.matrix_indices = { 0 };
		geoset.skin.assign(3 * 8, 0);
		for (size_t i = 0; i < geoset.skin.size(); i += 8) {
			geoset.skin[i] = 1;
			geoset.skin[i + 4] = 255;
			geoset.skin[i + 1] = 9;
			geoset.skin[i + 5] = 0;
		}
		CHECK(model.is_valid());
	}
}

TEST_CASE("Validator: a name MDL cannot quote is reported") {
	// MDL has no escape syntax, so a name with a quote in it is written as Bone ""Harold"" { and
	// nothing reads that back — not us, and not Blizzard's own converter, which emits the same text.
	// 36 models on Hiveworkshop are named this way.
	mdx::MDX model = model_with_bones({ { 0, -1 } });
	model.bones[0].node.name = "\"Harold\"";
	model.sequences.push_back(mdx::Sequence { .name = "line\nbreak" });

	const auto messages = model.validate();
	const auto reported = [&](std::string_view fragment) {
		return has_message(messages, mdx::ValidationSeverity::warning, fragment);
	};
	CHECK(reported("Node"));
	CHECK(reported("Sequence"));

	// A clean name is not reported.
	mdx::MDX clean = model_with_bones({ { 0, -1 } });
	clean.bones[0].node.name = "Harold";
	const auto clean_messages = clean.validate();
	CHECK(!std::ranges::any_of(clean_messages, [](const mdx::ValidationMessage& message) {
		return message.message.contains("cannot be written to MDL");
	}));
}

TEST_CASE("Validator: a boneless model may skin to anything") {
	// is_valid() runs immediately before fix_up() everywhere, and fix_up() rewrites a boneless model's
	// matrix indices onto the bone it adds, so none of them can stop it rendering. The game agrees:
	// it draws such a model unskinned and never reads the indices, so object ids 0, 1 and 200 all
	// render identically in the World Editor. merge_with() builds this, as do ~100 Hiveworkshop
	// uploads and the 267 CORN-only models in the dev build.
	mdx::MDX model {};
	auto& geoset = model.geosets.emplace_back();
	give_material(model);
	geoset.vertices.assign(3, glm::vec3(0.f));
	geoset.vertex_groups.assign(3, 0);
	geoset.matrix_groups = { 1 };

	for (const uint32_t index : { 0u, 1u, 200u }) {
		model.geosets[0].matrix_indices = { index };
		CHECK(model.is_valid());
	}
}

TEST_CASE("Validator: an SD geoset needs one vertex group per vertex") {
	// matrix_groups_as_skin_weights() builds one entry per vertex group, and
	// Skeleton::build_sample_frames then reads it by vertex, so a short list runs off the end.
	mdx::MDX model = model_with_bones({ { 0, -1 } });
	auto& geoset = model.geosets.emplace_back();
	give_material(model);
	geoset.vertices.assign(3, glm::vec3(0.f));
	geoset.matrix_groups = { 1 };
	geoset.matrix_indices = { 0 };

	SUBCASE("one group per vertex") {
		geoset.vertex_groups.assign(3, 0);
		CHECK(model.is_valid());
		CHECK(!has_message(model.validate(), mdx::ValidationSeverity::error, "vertex groups but"));
	}

	SUBCASE("fewer groups than vertices") {
		geoset.vertex_groups.assign(2, 0);
		CHECK(!model.is_valid());
		CHECK(has_message(model.validate(), mdx::ValidationSeverity::error, "has 2 vertex groups but 3 vertices"));
	}

	SUBCASE("no groups at all") {
		CHECK(!model.is_valid());
		CHECK(has_message(model.validate(), mdx::ValidationSeverity::error, "has 0 vertex groups but 3 vertices"));
	}

	SUBCASE("an HD geoset skins from its skin weights, so the list is only vestigial there") {
		// 5017 of the 12066 models in the 2.1 dev build carry a mismatched list on an HD geoset.
		geoset.vertex_groups.assign(2, 0);
		geoset.skin.assign(3 * 8, 0);
		for (size_t i = 0; i < geoset.skin.size(); i += 8) {
			geoset.skin[i + 4] = 255;
		}
		CHECK(model.is_valid());
		CHECK(has_message(model.validate(), mdx::ValidationSeverity::warning, "vertex groups but"));
	}
}
