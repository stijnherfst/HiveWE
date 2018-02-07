#include "stdafx.h"

Hierarchy hierarchy;

void Hierarchy::init() {
	war3Patch.open(warcraft_directory / L"War3Patch.mpq", STREAM_FLAG_READ_ONLY);
	war3xLocal.open(warcraft_directory / L"War3xlocal.mpq", STREAM_FLAG_READ_ONLY);
	war3x.open(warcraft_directory / L"War3x.mpq", STREAM_FLAG_READ_ONLY);
	war3Local.open(warcraft_directory / L"War3local.mpq", STREAM_FLAG_READ_ONLY);
	war3.open(warcraft_directory / L"War3.mpq", STREAM_FLAG_READ_ONLY);
}

void Hierarchy::load_tileset(char tileset_code) {
	std::string file_name = tileset_code + ".mpq"s;

	mpq::File tileset_mpq;
	if (war3Patch.file_exists(file_name)) {
		tileset_mpq = war3Patch.file_open(file_name);
	} else if (war3xLocal.file_exists(file_name)) {
		tileset_mpq = war3xLocal.file_open(file_name);
	} else if (war3x.file_exists(file_name)) {
		tileset_mpq = war3x.file_open(file_name);
	} else if (war3Local.file_exists(file_name)) {
		tileset_mpq = war3Local.file_open(file_name);
	} else if (war3.file_exists(file_name)) {
		tileset_mpq = war3.file_open(file_name);
	}

	tileset.open(tileset_mpq);
}

BinaryReader Hierarchy::open_file(const std::string path) {
	if (tileset.handle == nullptr) {
		std::cout << "Hierarchy tileset has not been initialised" << std::endl;
	}

	mpq::File file;
	if (tileset.file_exists(path)) {
		file = tileset.file_open(path);
	} else if (map.file_exists(path)) {
		file = map.file_open(path);
	} else if (war3Patch.file_exists(path)) {
		file = war3Patch.file_open(path);
	} else if (war3xLocal.file_exists(path)) {
		file = war3xLocal.file_open(path);
	} else if (war3x.file_exists(path)) {
		file = war3x.file_open(path);
	} else if (war3Local.file_exists(path)) {
		file = war3Local.file_open(path);
	} else if (war3.file_exists(path)) {
		file = war3.file_open(path);
	} else {
		std::cout << "Unable to find file in hierarchy";
		return std::vector<uint8_t>();
	}

	return BinaryReader(file.read());
}

bool Hierarchy::file_exists(const fs::path path) {
	return tileset.file_exists(path)
		|| map.file_exists(path)
		|| war3Patch.file_exists(path)
		|| war3xLocal.file_exists(path)
		|| war3x.file_exists(path)
		|| war3Local.file_exists(path)
		|| war3.file_exists(path);
}