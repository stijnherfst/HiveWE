#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <doctest/doctest.h>

import MDX;

namespace {
	std::string read_text_file(const std::filesystem::path& path) {
		std::ifstream f(path, std::ios::binary);
		REQUIRE(f.is_open());
		std::stringstream ss;
		ss << f.rdbuf();
		return ss.str();
	}

	/// Parse a fixture .mdl and return its validation messages. from_mdl does not run fix_up,
	/// so the deliberate defect in each fixture survives to validate().
	std::vector<mdx::ValidationMessage> validate_fixture(const char* relative_path) {
		const std::filesystem::path fixture = std::filesystem::path(MDL_FIXTURES_DIR) / relative_path;
		const std::string source = read_text_file(fixture);
		auto parsed = mdx::MDX::from_mdl(source);
		REQUIRE(parsed.has_value());
		return parsed.value().validate();
	}

	bool has_message(const std::vector<mdx::ValidationMessage>& messages, mdx::ValidationSeverity severity, std::string_view needle) {
		return std::ranges::any_of(messages, [&](const mdx::ValidationMessage& message) {
			return message.severity == severity && message.message.find(needle) != std::string_view::npos;
		});
	}
}

TEST_CASE("Validator: geoset references invalid material id") {
	const auto messages = validate_fixture("bad_geoset_material_id_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "invalid material id 5"));
}

TEST_CASE("Validator: geoset animation references invalid geoset id") {
	const auto messages = validate_fixture("bad_geosetanim_geoset_id_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "Geoset animation 0 references invalid geoset id 9"));
}

TEST_CASE("Validator: bone references invalid geoset id") {
	const auto messages = validate_fixture("bad_bone_geoset_id_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "Bone 0 references invalid geoset id 7"));
}

TEST_CASE("Validator: layer references invalid texture id (fatal)") {
	const auto messages = validate_fixture("bad_layer_texture_id_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::error, "invalid texture id 9"));
}

TEST_CASE("Validator: geoset face index exceeds vertex count") {
	const auto messages = validate_fixture("bad_face_index_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "face index 9 that exceeds vertex count 3"));
}

TEST_CASE("Validator: geoset normal count mismatch") {
	const auto messages = validate_fixture("mismatched_normals_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "2 normals but 3 vertices"));
}

TEST_CASE("Validator: particle emitter 2 references invalid texture id") {
	const auto messages = validate_fixture("bad_emitter2_texture_id_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "invalid texture id 4"));
}

TEST_CASE("Validator: ribbon emitter references invalid material id") {
	const auto messages = validate_fixture("bad_ribbon_material_id_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "invalid material id 3"));
}

TEST_CASE("Validator: event object references invalid global sequence id") {
	const auto messages = validate_fixture("bad_eventobject_globalseq_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "Event object \"SNDxFOO0\" references invalid global sequence id 5"));
}

TEST_CASE("Validator: track references invalid global sequence id") {
	const auto messages = validate_fixture("bad_track_globalseq_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "A track references invalid global sequence id 5"));
}

TEST_CASE("Validator: texture has no replaceable id and no path") {
	const auto messages = validate_fixture("missing_texture_path_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "no replaceable id and no file path"));
}

TEST_CASE("Validator: skin weight count mismatch") {
	const auto messages = validate_fixture("bad_skinweights_v900.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "2 skin weights but 3 vertices"));
}

TEST_CASE("Validator: pivot count does not match node count") {
	const auto messages = validate_fixture("bad_pivot_count_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "2 pivot points but 1 nodes"));
}

TEST_CASE("Validator: sequence has a non-positive interval") {
	const auto messages = validate_fixture("bad_sequence_interval_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "non-positive interval"));
}

// ——————————————————————— Batch 2 ———————————————————————

TEST_CASE("Validator: textures") {
	const auto messages = validate_fixture("textures_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::error, "corrupted path"));
	CHECK(has_message(messages, mdx::ValidationSeverity::error, "unknown replaceable id 99"));
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "both a replaceable id"));
	CHECK(has_message(messages, mdx::ValidationSeverity::unused, "Texture 3 is never referenced"));
}

TEST_CASE("Validator: geometry") {
	const auto messages = validate_fixture("geometry_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "not a multiple of 3"));
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "sequence extents but the model has"));
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "referenced by 2 geoset animations"));
}

TEST_CASE("Validator: SD matrix skinning") {
	const auto messages = validate_fixture("matrix_skinning_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::error, "matrix index 9 references a node that does not exist"));
	CHECK(has_message(messages, mdx::ValidationSeverity::severe, "matrix index 1 references a node that is not a bone"));
}

TEST_CASE("Validator: HD skin weights") {
	const auto messages = validate_fixture("skin_v900.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::error, "attached to object 5 which does not exist"));
	CHECK(has_message(messages, mdx::ValidationSeverity::severe, "attached to object 1 which is not a bone"));
	CHECK(has_message(messages, mdx::ValidationSeverity::severe, "not normalized"));
}

TEST_CASE("Validator: sequences") {
	const auto messages = validate_fixture("sequences_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::severe, "starts before sequence 0"));
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "Global sequence 0 has zero duration"));
}

TEST_CASE("Validator: node hierarchy") {
	const auto messages = validate_fixture("nodes_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::severe, "part of a parent cycle"));
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "has no vertices attached"));
}

TEST_CASE("Validator: objects") {
	const auto messages = validate_fixture("objects_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "attenuation start"));
	CHECK(has_message(messages, mdx::ValidationSeverity::error, "Attachment \"Ref\" has an invalid path"));
	CHECK(has_message(messages, mdx::ValidationSeverity::error, "has an invalid model path"));
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "invalid clip planes"));
}

TEST_CASE("Validator: particle emitter 2") {
	const auto messages = validate_fixture("emitter2_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::error, "invalid replaceable id 99"));
	CHECK(has_message(messages, mdx::ValidationSeverity::severe, "time middle"));
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "zero rows or columns"));
}

TEST_CASE("Validator: event objects") {
	const auto messages = validate_fixture("events_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::error, "has no event tracks"));
	CHECK(has_message(messages, mdx::ValidationSeverity::severe, "not strictly increasing"));
}

TEST_CASE("Validator: animation tracks") {
	const auto messages = validate_fixture("tracks_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::severe, "not in ascending order"));
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "duplicate keyframe"));
}

TEST_CASE("Validator: bind pose count") {
	const auto messages = validate_fixture("bind_pose_v900.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "bind pose matrices but expected"));
}

TEST_CASE("Validator: unused material and texture animation") {
	const auto messages = validate_fixture("unused_material_textureanim_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::unused, "Material 1 is never referenced"));
	CHECK(has_message(messages, mdx::ValidationSeverity::unused, "Texture animation 0 is never referenced"));
}

TEST_CASE("Validator: negative extents") {
	const auto messages = validate_fixture("negative_extent_v800.mdl");
	CHECK(has_message(messages, mdx::ValidationSeverity::warning, "Model has negative extents"));
}

TEST_CASE("Validator: SD geoset too many vertices (in-memory)") {
	mdx::MDX model;
	model.version = 800;

	mdx::Material material;
	mdx::Layer layer;
	layer.shader = mdx::ShaderType::SD;
	material.layers.push_back(layer);
	model.materials.push_back(material);

	mdx::Geoset geoset;
	geoset.material_id = 0;
	geoset.vertices.resize(8000); // Above the SD limit (7433).
	model.geosets.push_back(geoset);

	const auto messages = model.validate();
	CHECK(has_message(messages, mdx::ValidationSeverity::severe, "exceeds the Warcraft 3 limit"));
}
