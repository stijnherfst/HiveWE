#include <doctest/doctest.h>

import std;
import types;
import no_init_allocator;
import BinaryReader;
import BinaryWriter;
import SLK;
import ModificationTables;

using namespace std::string_literals;

constexpr u32 mod_table_version = 3;

TEST_CASE("save_modification_table data repeat") {
	slk::SLK meta;
	meta.add_row("Ocr6");
	meta.set_shadow_data("field", "Ocr6", "data");
	meta.set_shadow_data("repeat", "Ocr6", "4"); // non-zero indicates repeating
	meta.set_shadow_data("data", "Ocr6", "1");
	meta.set_shadow_data("type", "Ocr6", "int");
	meta.build_meta_map();

	slk::SLK data;
	data.add_row("Test");
	data.set_shadow_data("dataa1", "Test", "67");

	BinaryWriter writer;
	save_modification_table(writer, data, meta, false, true, false);

	slk::SLK loaded;
	BinaryReader reader(writer.buffer);
	load_modification_table(reader, mod_table_version, loaded, meta, false, true);

	CHECK(loaded.data("dataa1", "Test") == "67");
}
