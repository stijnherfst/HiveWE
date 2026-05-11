#include <doctest/doctest.h>

import std;
import types;
import no_init_allocator;
import BinaryWriter;

TEST_CASE("write_c_string_padded pads shorter string with null bytes") {
	BinaryWriter writer;
	writer.write_c_string_padded("hi", 8);

	REQUIRE(writer.buffer.size() == 8);
	CHECK(writer.buffer[0] == 'h');
	CHECK(writer.buffer[1] == 'i');
	for (size_t i = 2; i < 8; ++i) {
		CHECK(writer.buffer[i] == '\0');
	}
}

TEST_CASE("write_c_string_padded truncates string longer than final_size") {
	BinaryWriter writer;
	writer.write_c_string_padded("abcdef", 3);

	REQUIRE(writer.buffer.size() == 3);
	CHECK(writer.buffer[0] == 'a');
	CHECK(writer.buffer[1] == 'b');
	CHECK(writer.buffer[2] == 'c');
}

TEST_CASE("write_c_string_padded exact fit writes no trailing null") {
	BinaryWriter writer;
	writer.write_c_string_padded("abc", 3);

	REQUIRE(writer.buffer.size() == 3);
	CHECK(writer.buffer[0] == 'a');
	CHECK(writer.buffer[1] == 'b');
	CHECK(writer.buffer[2] == 'c');
}

TEST_CASE("write_c_string_padded empty string yields all null padding") {
	BinaryWriter writer;
	writer.write_c_string_padded("", 4);

	REQUIRE(writer.buffer.size() == 4);
	for (u8 byte : writer.buffer) {
		CHECK(byte == '\0');
	}
}

TEST_CASE("write_c_string_padded preserves prior buffer contents and nulls only the padding region") {
	BinaryWriter writer;
	writer.write<u32>(0xDEADBEEF);
	writer.write_c_string_padded("ok", 6);

	REQUIRE(writer.buffer.size() == 4 + 6);

	u32 leading{};
	std::memcpy(&leading, writer.buffer.data(), sizeof(leading));
	CHECK(leading == 0xDEADBEEF);

	CHECK(writer.buffer[4] == 'o');
	CHECK(writer.buffer[5] == 'k');
	for (size_t i = 6; i < 10; ++i) {
		CHECK(writer.buffer[i] == '\0');
	}
}

// Regression test: buffer uses default_init_allocator, so resize() leaves new bytes
// uninitialized. write_c_string_padded must therefore explicitly null-fill the padding
// rather than relying on resize() to zero memory.
TEST_CASE("write_c_string_padded null-fills padding even when underlying storage is dirty") {
	BinaryWriter writer;
	writer.buffer.reserve(64);
	for (size_t i = 0; i < 32; ++i) {
		writer.buffer.push_back(0xAB);
	}
	writer.buffer.clear();

	writer.write_c_string_padded("x", 16);

	REQUIRE(writer.buffer.size() == 16);
	CHECK(writer.buffer[0] == 'x');
	for (size_t i = 1; i < 16; ++i) {
		CHECK(writer.buffer[i] == '\0');
	}
}

TEST_CASE("write_vector with std::string concatenates without separators") {
	const std::vector<std::string> source{"foo", "bar"};

	BinaryWriter writer;
	writer.write_vector(source);

	REQUIRE(writer.buffer.size() == 6);
	const std::string result(writer.buffer.begin(), writer.buffer.end());
	CHECK(result == "foobar");
}