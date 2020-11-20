#include "Hierarchy.h"

#include <QSettings>

#include <iostream>
#include <fstream>

#include "no_init_allocator.h"

using namespace std::literals::string_literals;

Hierarchy hierarchy;

void Hierarchy::open_casc(fs::path directory) {
	warcraft_directory = directory;
	QSettings settings;
	ptr = settings.value("type", "Retail").toString().toStdString() != "Retail";
	hd = settings.value("hd", "True").toString() != "False";
	teen = settings.value("teen", "False").toString() != "False";

	std::cout << "Loading CASC data from: " << warcraft_directory << "\n";
	game_data.open(warcraft_directory / "Data", ptr);
	root_directory = warcraft_directory / (ptr ? "_ptr_" : "_retail_");
	aliases.load("filealiases.json");
}

BinaryReader Hierarchy::open_file(const fs::path& path) const {
	casc::File file;

	if (hd && teen && map_file_exists("_hd.w3mod:_teen.w3mod:" + path.string())) {
		return map_file_read("_hd.w3mod:_teen.w3mod:" + path.string());
	} else if (hd && map_file_exists("_hd.w3mod:" + path.string())) {
		return map_file_read("_hd.w3mod:" + path.string());
	}  else if (map_file_exists(path)) {
		return map_file_read(path);
	} else if (fs::exists(root_directory / path)) {
		std::ifstream stream(root_directory / path, std::ios::binary);
		return BinaryReader(std::vector<uint8_t, default_init_allocator<uint8_t>>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()));
	} else if (hd && game_data.file_exists("war3.w3mod:_hd.w3mod:_tilesets/"s + tileset + ".w3mod:"s + path.string())) {
		file = game_data.file_open("war3.w3mod:_hd.w3mod:_tilesets/"s + tileset + ".w3mod:"s + path.string());
	} else if (hd && teen && game_data.file_exists("war3.w3mod:_hd.w3mod:_teen.w3mod:"s + path.string())) {
		file = game_data.file_open("war3.w3mod:_hd.w3mod:_teen.w3mod:"s + path.string());
	} else if (hd && game_data.file_exists("war3.w3mod:_hd.w3mod:"s + path.string())) {
		file = game_data.file_open("war3.w3mod:_hd.w3mod:"s + path.string());
	} else if (game_data.file_exists("war3.w3mod:_tilesets/"s + tileset + ".w3mod:"s + path.string())) {
		file = game_data.file_open("war3.w3mod:_tilesets/"s + tileset + ".w3mod:"s + path.string());
	} else if (game_data.file_exists("war3.w3mod:_locales/enus.w3mod:"s + path.string())) {
		file = game_data.file_open("war3.w3mod:_locales/enus.w3mod:"s + path.string());
	} else if (teen && game_data.file_exists("war3.w3mod:_teen.w3mod:"s + path.string())) {
		file = game_data.file_open("war3.w3mod:_teen.w3mod:"s + path.string());
	} else if (game_data.file_exists("war3.w3mod:"s + path.string())) {
		file = game_data.file_open("war3.w3mod:"s + path.string());
	} else if (game_data.file_exists("war3.w3mod:_deprecated.w3mod:"s + path.string())) {
		file = game_data.file_open("war3.w3mod:_deprecated.w3mod:"s + path.string());
	} else if (aliases.exists(path.string())) {
		return open_file(aliases.alias(path.string()));
	} else {
		throw std::invalid_argument(path.string() + " could not be found in the hierarchy");
	}

	return BinaryReader(file.read());
}

bool Hierarchy::file_exists(const fs::path& path) const {
	if (path.empty()) {
		return false;
	}

	return (hd && teen && map_file_exists("_hd.w3mod:_teen.w3mod:" + path.string()))
		|| (hd && map_file_exists("_hd.w3mod:" + path.string()))
		|| map_file_exists(path)
		|| fs::exists(root_directory / path)
		|| (hd && game_data.file_exists("war3.w3mod:_hd.w3mod:_tilesets/"s + tileset + ".w3mod:"s + path.string()))
		|| (hd && teen && game_data.file_exists("war3.w3mod:_hd.w3mod:_teen.w3mod:"s + path.string()))
		|| (hd && game_data.file_exists("war3.w3mod:_hd.w3mod:"s + path.string()))
		|| game_data.file_exists("war3.w3mod:_tilesets/"s + tileset + ".w3mod:"s + path.string())
		|| game_data.file_exists("war3.w3mod:_locales/enus.w3mod:"s + path.string())
		|| (teen && game_data.file_exists("war3.w3mod:_teen.w3mod:"s + path.string()))
		|| game_data.file_exists("war3.w3mod:"s + path.string())
		|| game_data.file_exists("war3.w3mod:_deprecated.w3mod:"s + path.string())
		|| (aliases.exists(path.string()) ? file_exists(aliases.alias(path.string())) : false );
}

BinaryReader Hierarchy::map_file_read(const fs::path& path) const {
	std::ifstream stream(map_directory / path, std::ios::binary);
	return BinaryReader(std::vector<uint8_t, default_init_allocator<uint8_t>>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()));
}

/// source somewhere on disk, destination relative to the map
void Hierarchy::map_file_add(const fs::path& source, const fs::path& destination) const {
	fs::copy_file(source, map_directory / destination, fs::copy_options::overwrite_existing);
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