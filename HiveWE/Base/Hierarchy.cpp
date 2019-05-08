#include "stdafx.h"

Hierarchy hierarchy;

void Hierarchy::open_casc(fs::path directory) {
	warcraft_directory = directory;
	std::cout << "Loading CASC data from: " << warcraft_directory << "\n";
	game_data.open(warcraft_directory / "Data");
	aliases.load("filealiases.json");
}

BinaryReader Hierarchy::open_file(const fs::path& path) const {
	casc::File file;

	if (map_file_exists(path)) {
		return map_file_read(path);
	} else if (fs::exists(warcraft_directory / "War3Mod.mpq" / path)) {
		std::ifstream fin(warcraft_directory / "War3Mod.mpq" / path, std::ios_base::binary);
		fin.seekg(0, std::ios::end);
		const size_t fileSize = fin.tellg();
		fin.seekg(0, std::ios::beg);
		std::vector<uint8_t> buffer(fileSize);
		fin.read(reinterpret_cast<char*>(buffer.data()), fileSize);
		fin.close();
		return BinaryReader(buffer);
	} else if (game_data.file_exists("war3.mpq:"s + tileset + ".mpq:"s + path.string())) {
		file = game_data.file_open("war3.mpq:"s + tileset + ".mpq:"s + path.string());
	} else if (game_data.file_exists("enus-war3local.mpq:"s + path.string())) {
		file = game_data.file_open("enus-war3local.mpq:"s + path.string());
	} else if (game_data.file_exists("war3.mpq:"s + path.string())) {
		file = game_data.file_open("war3.mpq:"s + path.string());
	} else if (game_data.file_exists("deprecated.mpq:"s + path.string())) {
		file = game_data.file_open("deprecated.mpq:"s + path.string());
	} else {
		if (aliases.exists(path.string())) {
			return open_file(aliases.alias(path.string()));
		} else {
			throw std::invalid_argument(path.string() + " could not be found in the hierarchy");
		}
	}
	return BinaryReader(file.read());
}

bool Hierarchy::file_exists(const fs::path& path) const {
	if (path.empty()) {
		return false;
	}

	return map_file_exists(path)
		|| fs::exists((warcraft_directory / "War3Mod.mpq") / path)
		|| game_data.file_exists("war3.mpq:"s + tileset + ".mpq:"s + path.string())
		|| game_data.file_exists("enus-war3local.mpq:"s + path.string())
		|| game_data.file_exists("war3.mpq:"s + path.string())
		|| game_data.file_exists("deprecated.mpq:"s + path.string())
		|| ((aliases.exists(path.string())) ? file_exists(aliases.alias(path.string())) : false );
}

BinaryReader Hierarchy::map_file_read(const fs::path& path) const {
	std::ifstream fin(map_directory / path, std::ios_base::binary);
	fin.seekg(0, std::ios::end);
	const size_t fileSize = fin.tellg();
	fin.seekg(0, std::ios::beg);
	std::vector<uint8_t> buffer(fileSize);
	fin.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	fin.close();
	return BinaryReader(buffer);
}

void Hierarchy::map_file_write(const fs::path& path, const std::vector<uint8_t>& data) const {
	std::ofstream outfile(map_directory / path, std::ios::binary);

	if (!outfile) { 
		throw std::runtime_error("Error writing file " + path.string());
	}

	outfile.write(reinterpret_cast<char const*>(data.data()), data.size());
}

void Hierarchy::map_file_remove(const fs::path& path) const {
	fs::remove(map_directory / path);
}

bool Hierarchy::map_file_exists(const fs::path& path) const {
	return fs::exists(map_directory / path);
}

void Hierarchy::map_file_rename(const fs::path& original, const fs::path& renamed) const {
	fs::rename(map_directory / original, map_directory / renamed);
}