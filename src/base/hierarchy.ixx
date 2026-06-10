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
	enum class FileSource {
		none = 0,
		overrides = 1 << 0,
		imports = 1 << 1,
		local_files = 1 << 2,
		casc = 1 << 3,
		all = overrides | imports | local_files | casc
	};

	char tileset = 'L';
	std::string locale = "enus";
	casc::CASC game_data;
	json::JSON aliases;

	fs::path map_directory;
	fs::path warcraft_directory;
	fs::path root_directory;

	bool ptr = false;
	bool hd = true;
	bool teen = false;
	bool allow_local_files = true;

	friend constexpr FileSource operator&(FileSource a, FileSource b);
	friend constexpr FileSource operator|(FileSource a, FileSource b);

	Hierarchy() {
		QSettings war3reg("HKEY_CURRENT_USER\\Software\\Blizzard Entertainment\\Warcraft III", QSettings::NativeFormat);
		allow_local_files = war3reg.value("Allow Local Files", 0).toInt() != 0;
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

	constexpr bool has_flag(Hierarchy::FileSource value, Hierarchy::FileSource flag) const {
		using U = std::underlying_type_t<Hierarchy::FileSource>;
		return (static_cast<U>(value) & static_cast<U>(flag)) != 0;
	}

	[[nodiscard]]
	/// Loads the file in the following order:
	/// 1. Editor overrides (data/overrides folder)
	/// 2. Map imports
	/// 3. Local files (if enabled)
	/// 4. Game casc archive (handles sd, hd and teen modes)
	auto open_file(const fs::path& path, const FileSource sources = FileSource::all) const -> std::expected<BinaryReader, std::string> {
		const auto path_str = path.generic_string();

#define TRY_OPEN(expr) \
	if (auto file = (expr); file) { \
		return file; \
	}

		if (has_flag(sources, FileSource::overrides)) {
			TRY_OPEN(read_file(fs::path("data/overrides") / path));
		}

		if (has_flag(sources, FileSource::imports)) {
			if (hd && teen) {
				TRY_OPEN(map_file_read(std::format("_hd.w3mod:_teen.w3mod:{}", path_str)));
			}

			if (hd) {
				TRY_OPEN(map_file_read(std::format("_hd.w3mod:{}", path_str)));
			}

			TRY_OPEN(map_file_read(path));
		}

		if (has_flag(sources, FileSource::local_files) && allow_local_files) {
			TRY_OPEN(read_file(root_directory / path));
		}

		if (has_flag(sources, FileSource::casc)) {
			if (hd) {
				TRY_OPEN(game_data.open_file(std::format("war3.w3mod:_hd.w3mod:_tilesets/{}.w3mod:{}", tileset, path_str)));
			}

			if (hd && teen) {
				TRY_OPEN(game_data.open_file(std::format("war3.w3mod:_hd.w3mod:_teen.w3mod:{}", path_str)));
			}

			if (hd) {
				TRY_OPEN(game_data.open_file(std::format("war3.w3mod:_hd.w3mod:{}", path_str)));
			}

			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:_tilesets/{}.w3mod:{}", tileset, path_str)));

			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:_locales/{}.w3mod:{}", locale, path_str)));

			if (teen) {
				TRY_OPEN(game_data.open_file(std::format("war3.w3mod:_teen.w3mod:{}", path_str)));
			}

			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:{}", path_str)));

			TRY_OPEN(game_data.open_file(std::format("war3.w3mod:_deprecated.w3mod:{}", path_str)));
		}

#undef TRY_OPEN

		if (aliases.exists(path_str)) {
			return open_file(aliases.alias(path_str));
		}

		return std::unexpected(std::format("{} could not be found in the hierarchy", path_str));
	}

	[[nodiscard]]
	bool file_exists(const fs::path& path) const {
		if (path.empty()) {
			return false;
		}

		const auto path_str = path.string();

		return fs::exists("data/overrides" / path) || (allow_local_files && fs::exists(root_directory / path))
			|| (hd && teen && map_file_exists("_hd.w3mod:_teen.w3mod:" + path_str)) || (hd && map_file_exists("_hd.w3mod:" + path_str))
			|| map_file_exists(path) || (hd && game_data.file_exists("war3.w3mod:_hd.w3mod:_tilesets/"s + tileset + ".w3mod:"s + path_str))
			|| (hd && teen && game_data.file_exists("war3.w3mod:_hd.w3mod:_teen.w3mod:"s + path_str))
			|| (hd && game_data.file_exists("war3.w3mod:_hd.w3mod:"s + path_str))
			|| game_data.file_exists("war3.w3mod:_tilesets/"s + tileset + ".w3mod:"s + path_str)
			|| game_data.file_exists(std::format("war3.w3mod:_locales/{}.w3mod:{}", locale, path_str))
			|| (teen && game_data.file_exists("war3.w3mod:_teen.w3mod:"s + path_str)) || game_data.file_exists("war3.w3mod:"s + path_str)
			|| game_data.file_exists("war3.w3mod:_deprecated.w3mod:"s + path_str)
			|| (aliases.exists(path_str) ? file_exists(aliases.alias(path_str)) : false);
	}

	[[nodiscard]]
	auto map_file_read(const fs::path& path) const -> std::expected<BinaryReader, std::string> {
		return read_file(map_directory / path);
	}

	/// source somewhere on disk, destination relative to the map
	void map_file_add(const fs::path& source, const fs::path& destination) const {
		fs::copy_file(source, map_directory / destination, fs::copy_options::overwrite_existing);
	}

	void map_file_write(const fs::path& path, const std::span<const u8> data) const {
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

using U = std::underlying_type_t<Hierarchy::FileSource>;

export constexpr Hierarchy::FileSource operator|(Hierarchy::FileSource a, Hierarchy::FileSource b) {
	using U = std::underlying_type_t<Hierarchy::FileSource>;
	return static_cast<Hierarchy::FileSource>(static_cast<U>(a) | static_cast<U>(b));
}

export constexpr Hierarchy::FileSource operator&(Hierarchy::FileSource a, Hierarchy::FileSource b) {
	using U = std::underlying_type_t<Hierarchy::FileSource>;
	return static_cast<Hierarchy::FileSource>(static_cast<U>(a) & static_cast<U>(b));
}

export constexpr Hierarchy::FileSource operator^(Hierarchy::FileSource a, Hierarchy::FileSource b) {
	using U = std::underlying_type_t<Hierarchy::FileSource>;
	return static_cast<Hierarchy::FileSource>(static_cast<U>(a) ^ static_cast<U>(b));
}

export constexpr Hierarchy::FileSource operator~(Hierarchy::FileSource a) {
	using U = std::underlying_type_t<Hierarchy::FileSource>;
	return static_cast<Hierarchy::FileSource>(~static_cast<U>(a));
}

export constexpr Hierarchy::FileSource& operator|=(Hierarchy::FileSource& a, Hierarchy::FileSource b) {
	a = a | b;
	return a;
}

export constexpr Hierarchy::FileSource& operator&=(Hierarchy::FileSource& a, Hierarchy::FileSource b) {
	a = a & b;
	return a;
}

export constexpr Hierarchy::FileSource& operator^=(Hierarchy::FileSource& a, Hierarchy::FileSource b) {
	a = a ^ b;
	return a;
}

export inline Hierarchy hierarchy;
