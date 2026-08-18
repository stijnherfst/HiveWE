// Adversarial models: inputs crafted to crash, hang, or slip a memory-unsafe value past MDX::is_valid()
// into fix_up() or the renderer. Each case pins one hardening fix. MDL is used where the defect can be
// written in text; the binary-only shapes live in fixtures/mdx and are read straight through the reader.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <doctest/doctest.h>

import MDX;
import BinaryReader;
import Utilities;

namespace {
	std::string read_text(const char* relative) {
		const std::filesystem::path path = std::filesystem::path(MDL_FIXTURES_DIR) / relative;
		std::ifstream f(path, std::ios::binary);
		REQUIRE_MESSAGE(f.is_open(), path.string());
		std::stringstream ss;
		ss << f.rdbuf();
		return ss.str();
	}

	BinaryReader read_mdx(const char* relative) {
		const std::filesystem::path path = std::filesystem::path(MDX_FIXTURES_DIR) / relative;
		auto buffer = read_file(path);
		REQUIRE_MESSAGE(buffer.has_value(), path.string());
		return std::move(buffer.value());
	}
}

// --- MDL text parser ---------------------------------------------------------------------------

TEST_CASE("Parser: a file truncated after a node header fails cleanly") {
	// The unknown-field error path read one token past the end; peek() now returns an EOF sentinel.
	const auto parsed = mdx::MDX::from_mdl(read_text("truncated_after_helper_v800.mdl"));
	CHECK(!parsed.has_value());
}

TEST_CASE("Parser: an enormous declared count is rejected, not pre-allocated") {
	// Vertices 4294967295 used to reserve tens of gigabytes before reading; the reserve is now capped.
	const auto parsed = mdx::MDX::from_mdl(read_text("huge_vertex_count_v800.mdl"));
	CHECK(!parsed.has_value());
}

// --- is_valid() gate ---------------------------------------------------------------------------

TEST_CASE("is_valid: a geoset material id past the material list is rejected") {
	auto parsed = mdx::MDX::from_mdl(read_text("material_id_no_materials_v800.mdl"));
	REQUIRE(parsed.has_value());
	CHECK(!parsed.value().is_valid());
}

TEST_CASE("is_valid: duplicate node ids are rejected") {
	auto parsed = mdx::MDX::from_mdl(read_text("nodeid_checksum_collision_v800.mdl"));
	REQUIRE(parsed.has_value());
	CHECK(!parsed.value().is_valid());
}

TEST_CASE("is_valid: a zero-size matrix group is rejected") {
	// Expanding a vertex's weight across a zero-size group divides 255 by zero.
	auto parsed = mdx::MDX::from_mdl(read_text("zero_matrix_group_v800.mdl"));
	REQUIRE(parsed.has_value());
	CHECK(!parsed.value().is_valid());
}

TEST_CASE("is_valid: a parent-id cycle is rejected") {
	// Two nodes parenting each other pass every per-node check but never reach a root.
	auto parsed = mdx::MDX::from_mdl(read_text("parent_cycle_v800.mdl"));
	REQUIRE(parsed.has_value());
	CHECK(!parsed.value().is_valid());
}

TEST_CASE("is_valid: a material layer with no textures is rejected") {
	// The renderer dereferences layer.textures[0]. Built in memory because the MDL path always injects
	// a default texture, so only a binary model can carry an empty layer.
	mdx::MDX model{};
	auto& bone = model.bones.emplace_back();
	bone.node.id = 0;
	bone.node.parent_id = -1;
	model.pivots.emplace_back(0.f, 0.f, 0.f);
	model.textures.emplace_back();
	auto& material = model.materials.emplace_back();
	material.layers.emplace_back(); // no textures
	auto& geoset = model.geosets.emplace_back();
	geoset.vertices.assign(3, glm::vec3(0.f));
	geoset.vertex_groups.assign(3, 0);
	geoset.matrix_groups = { 1 };
	geoset.matrix_indices = { 0 };
	geoset.material_id = 0;

	CHECK(!model.is_valid());
}

// --- fix_up() repairs (not gated: padded so a well-formed model is untouched) ------------------

TEST_CASE("fix_up: a normal buffer shorter than the vertices is padded") {
	auto parsed = mdx::MDX::from_mdl(read_text("short_normals_v800.mdl"));
	REQUIRE(parsed.has_value());
	mdx::MDX model = std::move(parsed.value());
	REQUIRE(model.geosets.size() == 1);
	model.fix_up();
	CHECK(model.geosets[0].normals.size() == model.geosets[0].vertices.size());
}

TEST_CASE("fix_up: a geoset with no texture coordinates gains a set sized to its vertices") {
	auto parsed = mdx::MDX::from_mdl(read_text("empty_uv_sets_v800.mdl"));
	REQUIRE(parsed.has_value());
	mdx::MDX model = std::move(parsed.value());
	REQUIRE(model.geosets.size() == 1);
	model.fix_up();
	REQUIRE(!model.geosets[0].uv_sets.empty());
	CHECK(model.geosets[0].uv_sets.front().size() == model.geosets[0].vertices.size());
}

TEST_CASE("fix_up: a skin buffer shorter than vertices*8 is padded") {
	auto parsed = mdx::MDX::from_mdl(read_text("short_skinweights_v900.mdl"));
	REQUIRE(parsed.has_value());
	mdx::MDX model = std::move(parsed.value());
	REQUIRE(model.geosets.size() == 1);
	model.fix_up();
	CHECK(model.geosets[0].skin.size() == model.geosets[0].vertices.size() * 8);
}

TEST_CASE("is_valid: an emitter with no object id is rejected") {
	// The MDL default object id is -1, which would index pivots[-1] in fix_up()'s extent recompute.
	// is_valid() catches it first, and every fix_up() caller - merge_with() included - gates on
	// is_valid(), so the bad id never reaches fix_up() rather than being defended against downstream.
	auto parsed = mdx::MDX::from_mdl(read_text("emitter_no_objectid_v800.mdl"));
	REQUIRE(parsed.has_value());
	CHECK(!parsed.value().is_valid());
}

// --- MDX binary reader -------------------------------------------------------------------------

TEST_CASE("Reader: an enormous track count throws instead of pre-allocating") {
	// KGTR track count patched to 0xFFFFFFFF: the reserve is capped, so the read runs off the buffer
	// and throws rather than requesting ~160 GB.
	BinaryReader reader = read_mdx("track_count_overflow_v800.mdx");
	CHECK_THROWS(mdx::MDX{ reader });
}

TEST_CASE("Reader: a BPOS matrix count that would overflow a 32-bit multiply parses safely") {
	// count*12 is now computed in 64-bit and clamped to the chunk size, so the stream stays in sync.
	BinaryReader reader = read_mdx("bpos_count_overflow_v900.mdx");
	CHECK_NOTHROW(mdx::MDX{ reader });
}

TEST_CASE("is_valid: matrix groups claiming more indices than MATS holds are rejected") {
	// MTGC group size patched to 4 while MATS holds one index: reading the group runs off matrix_indices.
	BinaryReader reader = read_mdx("matrix_indices_short_v800.mdx");
	mdx::MDX model{ reader };
	CHECK(!model.is_valid());
}
