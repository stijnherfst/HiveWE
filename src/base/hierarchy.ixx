module;

#include <QSettings>

export module Hierarchy;

import std;
import types;
import JSON;
import BinaryReader;
import CASC;
import no_init_allocator;
import Utilities;

using namespace std::literals::string_literals;
namespace fs = std::filesystem;

export class Hierarchy {
  public:
	char tileset = 'L';
	casc::CASC game_data;
	json::JSON aliases;

	fs::path map_directory;
	fs::path warcraft_directory;
	fs::path root_directory;

	bool ptr = false;
	bool hd = true;
	bool teen = false;
	bool local_files = true;

	Hierarchy() {
		QSettings war3reg("HKEY_CURRENT_USER\\Software\\Blizzard Entertainment\\Warcraft III", QSettings::NativeFormat);
		local_files = war3reg.value("Allow Local Files", 0).toInt() != 0;
	}

	bool open_casc(const fs::path& directory) {
		QSettings settings;
		ptr = settings.value("flavour", "Retail").toString() == "PTR";
		hd = settings.value("hd", "False").toString() == "True";
		teen = settings.value("teen", "False").toString() == "True";

		warcraft_directory = directory;

		bool open = game_data.open(warcraft_directory / (ptr ? ":w3t" : ":w3"));
		root_directory = warcraft_directory / (ptr ? "_ptr_" : "_retail_");

		if (open) {
			aliases.load(open_file("filealiases.json").value());
		}
		return open;
	}

	[[nodiscard]]
	auto open_file(const fs::path& path) const -> std::expected<BinaryReader, std::string> {
		const std::string path_str = path.string();

		using Candidate = std::function<std::expected<BinaryReader, std::string>()>;

		const std::vector<Candidate> candidates {
			[&] {
				return read_file("data/overrides" / path);
			},
			[&] {
				return local_files ? read_file(root_directory / path) : std::unexpected("skip");
			},
			[&] {
				return (hd && teen) ? map_file_read("_hd.w3mod:_teen.w3mod:" + path_str) : std::unexpected("skip");
			},
			[&] {
				return hd ? map_file_read("_hd.w3mod:" + path_str) : std::unexpected("skip");
			},
			[&] {
				return map_file_read(path);
			},
			[&] {
				return hd ? game_data.open_file(std::format("war3.w3mod:_hd.w3mod:_tilesets/{}.w3mod:{}", tileset, path_str))
						  : std::unexpected("skip");
			},
			[&] {
				return (hd && teen) ? game_data.open_file("war3.w3mod:_hd.w3mod:_teen.w3mod:"s + path_str) : std::unexpected("skip");
			},
			[&] {
				return hd ? game_data.open_file("war3.w3mod:_hd.w3mod:"s + path_str) : std::unexpected("skip");
			},
			[&] {
				return game_data.open_file(std::format("war3.w3mod:_tilesets/{}.w3mod:{}", tileset, path_str));
			},
			[&] {
				return game_data.open_file("war3.w3mod:_locales/enus.w3mod:"s + path_str);
			},
			[&] {
				return teen ? game_data.open_file("war3.w3mod:_teen.w3mod:"s + path_str) : std::unexpected("skip");
			},
			[&] {
				return game_data.open_file("war3.w3mod:"s + path_str);
			},
			[&] {
				return game_data.open_file("war3.w3mod:_deprecated.w3mod:"s + path_str);
			},
			[&] {
				return aliases.exists(path_str) ? open_file(aliases.alias(path_str)) : std::unexpected("skip");
			},
		};

		for (const auto& candidate : candidates) {
			if (const auto res = candidate(); res) {
				return res;
			}
		}

		return std::unexpected(path_str + " could not be found in the hierarchy");
	}

	bool file_exists(const fs::path& path) const {
		if (path.empty()) {
			return false;
		}

		return fs::exists("data/overrides" / path) || (local_files && fs::exists(root_directory / path))
			|| (hd && teen && map_file_exists("_hd.w3mod:_teen.w3mod:" + path.string()))
			|| (hd && map_file_exists("_hd.w3mod:" + path.string())) || map_file_exists(path)
			|| (hd && game_data.file_exists("war3.w3mod:_hd.w3mod:_tilesets/"s + tileset + ".w3mod:"s + path.string()))
			|| (hd && teen && game_data.file_exists("war3.w3mod:_hd.w3mod:_teen.w3mod:"s + path.string()))
			|| (hd && game_data.file_exists("war3.w3mod:_hd.w3mod:"s + path.string()))
			|| game_data.file_exists("war3.w3mod:_tilesets/"s + tileset + ".w3mod:"s + path.string())
			|| game_data.file_exists("war3.w3mod:_locales/enus.w3mod:"s + path.string())
			|| (teen && game_data.file_exists("war3.w3mod:_teen.w3mod:"s + path.string()))
			|| game_data.file_exists("war3.w3mod:"s + path.string())
			|| game_data.file_exists("war3.w3mod:_deprecated.w3mod:"s + path.string())
			|| (aliases.exists(path.string()) ? file_exists(aliases.alias(path.string())) : false);
	}

	[[nodiscard]]
	auto map_file_read(const fs::path& path) const -> std::expected<BinaryReader, std::string> {
		return read_file(map_directory / path);
	}

	/// source somewhere on disk, destination relative to the map
	void map_file_add(const fs::path& source, const fs::path& destination) const {
		fs::copy_file(source, map_directory / destination, fs::copy_options::overwrite_existing);
	}

	void map_file_write(const fs::path& path, const std::vector<u8>& data) const {
		std::ofstream outfile(map_directory / path, std::ios::binary);

		if (!outfile) {
			throw std::runtime_error("Error writing file " + path.string());
		}

		outfile.write(reinterpret_cast<char const*>(data.data()), data.size());
	}

	void map_file_remove(const fs::path& path) const {
		fs::remove(map_directory / path);
	}

	bool map_file_exists(const fs::path& path) const {
		return fs::exists(map_directory / path);
	}

	void map_file_rename(const fs::path& original, const fs::path& renamed) const {
		fs::rename(map_directory / original, map_directory / renamed);
	}
};

export inline Hierarchy hierarchy;
