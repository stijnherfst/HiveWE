#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdio>
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

	void roundtrip_fixture(const char* relative_path) {
		const std::filesystem::path fixture = std::filesystem::path(MDL_FIXTURES_DIR) / relative_path;
		const std::string source = read_text_file(fixture);

		// 1st pass: parse the fixture, then write.
		auto parsed_a = mdx::MDX::from_mdl(source);
		REQUIRE(parsed_a.has_value());
		const std::string out_a = parsed_a.value().to_mdl();

		// 2nd pass: parse the writer's output, then write again.
		auto parsed_b = mdx::MDX::from_mdl(out_a);
		REQUIRE(parsed_b.has_value());
		const std::string out_b = parsed_b.value().to_mdl();

		// Fixpoint: writing a parsed model must be stable.
		const bool same = out_a.size() == out_b.size()
			&& std::memcmp(out_a.data(), out_b.data(), out_a.size()) == 0;
		CHECK(same);
	}
}

TEST_CASE("MDL round-trip: minimal_v800.mdl") {
	roundtrip_fixture("minimal_v800.mdl");
}

TEST_CASE("MDL round-trip: minimal_v900.mdl") {
	roundtrip_fixture("minimal_v900.mdl");
}

TEST_CASE("MDL round-trip: minimal_v1000.mdl") {
	roundtrip_fixture("minimal_v1000.mdl");
}