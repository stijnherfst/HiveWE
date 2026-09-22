import std;

#include <doctest/doctest.h>

import MDX;
import BinaryReader;

namespace {
	std::string read_text_file(const std::filesystem::path& path) {
		std::ifstream f(path, std::ios::binary);
		REQUIRE(f.is_open());
		std::stringstream ss;
		ss << f.rdbuf();
		return ss.str();
	}

	/// MDL (file) -> MDX (memory) -> MDL (file)
	void roundtrip_fixture_mdl(const char* relative_path) {
		const std::filesystem::path fixture = std::filesystem::path(MDL_FIXTURES_DIR) / relative_path;
		const std::string source = read_text_file(fixture);

		// 1st pass: parse the fixture, then write.
		auto parsed_a = mdx::MDX::from_mdl(source);
		REQUIRE(parsed_a.has_value());
		const std::string out_a = parsed_a.value().to_mdl(parsed_a.value().version);

		CHECK_EQ(source.size(), out_a.size());
		CHECK_EQ(std::memcmp(source.data(), out_a.data(), source.size()), 0);
	}

	/// MDL (file) -> MDX (memory) -> MDX (file) -> MDX(memory) -> MDL (file)
	void roundtrip_fixture_mdx(const char* relative_path) {
		const std::filesystem::path fixture = std::filesystem::path(MDL_FIXTURES_DIR) / relative_path;
		const std::string source = read_text_file(fixture);

		auto parsed_a = mdx::MDX::from_mdl(source);
		REQUIRE(parsed_a.has_value());
		auto mdx = parsed_a.value().to_mdx(parsed_a.value().version);

		auto reader = BinaryReader(mdx.buffer);
		auto parsed_b = mdx::MDX(reader);
		auto out_a = parsed_b.to_mdl(parsed_b.version);

		CHECK_EQ(source.size(), out_a.size());
		CHECK_EQ(std::memcmp(source.data(), out_a.data(), source.size()), 0);
	}
}

TEST_CASE("MDL round-trip: minimal_v800.mdl") {
	roundtrip_fixture_mdl("minimal_v800.mdl");
	roundtrip_fixture_mdx("minimal_v800.mdl");
}

TEST_CASE("MDL round-trip: minimal_v900.mdl") {
	roundtrip_fixture_mdl("minimal_v900.mdl");
	roundtrip_fixture_mdx("minimal_v900.mdl");
}

TEST_CASE("MDL round-trip: minimal_v1000.mdl") {
	roundtrip_fixture_mdl("minimal_v1000.mdl");
	roundtrip_fixture_mdx("minimal_v1000.mdl");
}

TEST_CASE("MDL round-trip: minimal_v1100.mdl") {
	roundtrip_fixture_mdl("minimal_v1100.mdl");
	roundtrip_fixture_mdx("minimal_v1100.mdl");
}

TEST_CASE("MDL round-trip: minimal_v1800.mdl") {
	roundtrip_fixture_mdl("minimal_v1800.mdl");
	roundtrip_fixture_mdx("minimal_v1800.mdl");
}

// The scalar depth of field keywords are not fields, each is one keyframe at time 0. The game's
// own text reader stores FocalLength into the f-stop and FStop into the focal length, we don't.
TEST_CASE("MDL reader: scalar depth of field keywords become a single keyframe") {
	const std::string mdl =
		"Version {\n"
		"\tFormatVersion 1800,\n"
		"}\n"
		"Camera \"Portrait\" {\n"
		"\tPosition { 0, 0, 0 },\n"
		"\tFieldOfView 0.8,\n"
		"\tFarClip 1000,\n"
		"\tNearClip 0.1,\n"
		"\tDOFDistance 180,\n"
		"\tFocalLength 50,\n"
		"\tFStop 2.8,\n"
		"\tTarget {\n"
		"\t\tPosition { 0, 0, 0 },\n"
		"\t}\n"
		"}\n";

	auto parsed = mdx::MDX::from_mdl(mdl);
	REQUIRE(parsed.has_value());
	const std::string out = parsed.value().to_mdl(1800);
	CHECK(out.find("FocusDistanceKeys 1 {") != std::string::npos);
	CHECK(out.find("0: 180,") != std::string::npos);
	CHECK(out.find("FocalLengthKeys 1 {") != std::string::npos);
	CHECK(out.find("0: 50,") != std::string::npos);
	CHECK(out.find("FStopKeys 1 {") != std::string::npos);
	CHECK(out.find("0: 2.8,") != std::string::npos);
}

// The game's own binary reader stores every glider into the first entry, losing the rest. Ours
// reads the list as written.
TEST_CASE("MDX round-trip: every glider entry survives") {
	const std::string mdl =
		"Version {\n"
		"\tFormatVersion 1800,\n"
		"}\n"
		"Glider {\n"
		"\tGeosetId 3,\n"
		"}\n"
		"Glider {\n"
		"\tGeosetId 7,\n"
		"}\n";

	auto parsed = mdx::MDX::from_mdl(mdl);
	REQUIRE(parsed.has_value());
	auto mdx = parsed.value().to_mdx(1800);

	auto reader = BinaryReader(mdx.buffer);
	auto parsed_b = mdx::MDX(reader);
	const std::string out = parsed_b.to_mdl(1800);
	CHECK(out.find("Glider {\n\tGeosetId 3,\n}\n") != std::string::npos);
	CHECK(out.find("Glider {\n\tGeosetId 7,\n}\n") != std::string::npos);
}

// Sanity-check the version-aware MDL writer: writing a higher-version model out as
// a lower-version target must strip fields that the lower version does not understand.
TEST_CASE("MDL writer: cross-version downgrade strips version-specific fields") {
	const std::filesystem::path mdl_dir = std::filesystem::path(MDL_FIXTURES_DIR);

	{
		const std::string v900_src = read_text_file(mdl_dir / "minimal_v900.mdl");
		auto parsed = mdx::MDX::from_mdl(v900_src);
		REQUIRE(parsed.has_value());
		const std::string out_v800 = parsed.value().to_mdl(800);
		CHECK(out_v800.find("FormatVersion 800,") != std::string::npos);
		CHECK(out_v800.find("Shader \"") == std::string::npos);
		CHECK(out_v800.find("Tangents ") == std::string::npos);
		CHECK(out_v800.find("SkinWeights ") == std::string::npos);
		CHECK(out_v800.find("BindPose") == std::string::npos);
	}

	{
		const std::string v1000_src = read_text_file(mdl_dir / "minimal_v1000.mdl");
		auto parsed = mdx::MDX::from_mdl(v1000_src);
		REQUIRE(parsed.has_value());
		const std::string out_v900 = parsed.value().to_mdl(900);
		CHECK(out_v900.find("FormatVersion 900,") != std::string::npos);
		CHECK(out_v900.find("EmissiveGain") == std::string::npos);
		CHECK(out_v900.find("FresnelColor") == std::string::npos);
		CHECK(out_v900.find("FresnelOpacity") == std::string::npos);
		CHECK(out_v900.find("FresnelTeamColor") == std::string::npos);
	}

	{
		const std::string v1800_src = read_text_file(mdl_dir / "minimal_v1800.mdl");
		auto parsed = mdx::MDX::from_mdl(v1800_src);
		REQUIRE(parsed.has_value());
		const std::string out_v1200 = parsed.value().to_mdl(1200);
		CHECK(out_v1200.find("FormatVersion 1200,") != std::string::npos);
		// Shadow intensity arrived at 1200, the rest of the light fields later
		CHECK(out_v1200.find("static ShadowIntensity 0.4,") != std::string::npos);
		CHECK(out_v1200.find("ShadowCasting,") == std::string::npos);
		CHECK(out_v1200.find("ShadowCastingStart") == std::string::npos);
		CHECK(out_v1200.find("ShadowCastingEnd") == std::string::npos);
		CHECK(out_v1200.find("QuadraticFalloff") == std::string::npos);
		CHECK(out_v1200.find("LinearFalloff") == std::string::npos);
		CHECK(out_v1200.find("Damping") == std::string::npos);
	}
}
