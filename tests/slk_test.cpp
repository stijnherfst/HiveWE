#include <doctest/doctest.h>

import std;
import SLK;

namespace fs = std::filesystem;

/// Retail files such as Doodads.slk skip a row number entirely, leaving a hole in the row indices.
TEST_CASE("SLK load skipped row number") {
	slk::SLK slk;
	const auto result = slk.load_local(fs::path(SLK_FIXTURES_DIR) / "skipped_row.slk");

	REQUIRE(result.has_value());

	CHECK(slk.rows() == 3);
	CHECK(slk.index_to_row.size() == 3);
	CHECK(slk.index_to_row.at(0) == "AAAA");
	CHECK(slk.index_to_row.at(1) == "BBBB");
	CHECK(slk.index_to_row.at(2) == "DDDD");

	CHECK(slk.row_headers.at("AAAA") == 0);
	CHECK(slk.row_headers.at("BBBB") == 1);
	CHECK(slk.row_headers.at("DDDD") == 2);

	CHECK(slk.data("name", "DDDD") == "Delta");
}

/// A row can hold data cells without a row header. Those rows are dropped and the remaining indices closed up.
TEST_CASE("SLK load row without a header") {
	slk::SLK slk;
	const auto result = slk.load_local(fs::path(SLK_FIXTURES_DIR) / "headerless_row.slk");

	REQUIRE(result.has_value());

	CHECK(slk.rows() == 2);
	CHECK(slk.index_to_row.size() == 2);
	CHECK(slk.index_to_row.at(0) == "AAAA");
	CHECK(slk.index_to_row.at(1) == "CCCC");

	CHECK(slk.row_headers.at("AAAA") == 0);
	CHECK(slk.row_headers.at("CCCC") == 1);
	CHECK(!slk.row_headers.contains(""));

	CHECK(slk.data("name", "CCCC") == "Gamma");
}
