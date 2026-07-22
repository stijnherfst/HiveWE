export module Protection;

import std;

namespace fs = std::filesystem;

namespace protection {
	export struct ProtectionSettings {
		bool delete_editor_files = true;
		bool remove_metadata = true;
		bool obfuscate_script = false;
		/// Prepend junk so the MPQ header no longer sits at offset 0
		/// StormLib has no issues with this so only stops other more incomplete parsers
		bool junk_header_offset = false;
		/// Encrypt imported (non-standard game) files inside the archive
		/// The encryption key is the file path. Because we delete the list file the files in the archive can't be easily dumped
		/// StormLib cracked this so only stops other more incomplete parsers
		bool encrypt_imports = false;
	};

	/// Editor only files the game doesn't need
	constexpr std::array world_editor_files = {
		"war3map.doo",
		"war3map.w3c",
		"war3map.w3s",
		"war3map.imp",
		"war3mapUnits.doo",
		"war3map.w3r",
		"war3map.wct",
		"war3map.wtg",
		"conversation.json",
		"hiveWE/terrain_pathing.json",
	};

	/// Whether only the editor needs this file, matched against the path relative to the map root
	export bool is_editor_file(const fs::path& relative_path) {
		const std::string name = relative_path.generic_string();
		return std::ranges::any_of(world_editor_files, [&](const char* editor_file) {
			return name == editor_file;
		});
	}

	/// Fixed game/editor files that the client opens by their exact name. Everything else in the
	/// map folder is an imported asset. Kept in sync with Imports::blacklist.
	constexpr std::array standard_game_files = {
		"war3map.doo",		"war3map.imp",	   "war3map.j",		  "war3map.lua",	 "war3map.mmp",		   "war3map.shd",
		"war3map.w3a",		"war3map.w3b",	   "war3map.w3c",	  "war3map.w3d",	 "war3map.w3e",		   "war3map.w3h",
		"war3map.w3i",		"war3map.w3q",	   "war3map.w3r",	  "war3map.w3t",	 "war3map.w3u",		   "war3map.wct",
		"war3map.wpm",		"war3map.wtg",	   "war3map.wts",	  "war3map.w3s",	 "war3mapUnits.doo",   "war3mapMap.blp",
		"war3mapExtra.txt", "war3mapMisc.txt", "war3mapSkin.txt", "war3mapSkin.w3a", "war3mapSkin.w3b",	   "war3mapSkin.w3d",
		"war3mapSkin.w3h",	"war3mapSkin.w3q", "war3mapSkin.w3t", "war3mapSkin.w3u", "war3mapPreview.tga",
	};

	/// Whether the game opens this file by its fixed name (so it must stay unencrypted and listed).
	/// The complement is treated as an imported asset, eligible for encryption.
	export bool is_standard_game_file(const fs::path& relative_path) {
		const std::string name = relative_path.filename().string();
		return std::ranges::any_of(standard_game_files, [&](const char* game_file) {
			return name == game_file;
		});
	}

	/// Prepends `blocks * 512` bytes of junk to a finished MPQ so its header no longer sits at
	/// offset 0. Warcraft III and StormLib locate the header by scanning 512-byte boundaries, so
	/// the archive still opens; tools that assume offset 0 fail. Block table offsets are relative
	/// to the header, so the contents stay valid. Returns false on any IO error.
	export bool prepend_junk_header(const fs::path& archive_path, const size_t blocks = 1) {
		std::error_code ec;
		const auto size = fs::file_size(archive_path, ec);
		if (ec) {
			return false;
		}

		std::vector<char> data(static_cast<size_t>(size));
		{
			std::ifstream in(archive_path, std::ios::binary);
			if (!in.read(data.data(), data.size()) && size != 0) {
				return false;
			}
		}

		const size_t junk_size = blocks * 512;
		std::vector<char> junk(junk_size);
		std::mt19937 rng(std::random_device {}());
		std::uniform_int_distribution<int> byte(0, 255);
		for (char& c : junk) {
			c = static_cast<char>(byte(rng));
		}
		// Never let a 512-aligned position in the junk look like an MPQ/userdata header,
		// otherwise the boundary scan would stop on the decoy instead of the real archive.
		for (size_t offset = 0; offset < junk_size; offset += 512) {
			junk[offset] = 0x00;
		}

		std::ofstream out(archive_path, std::ios::binary | std::ios::trunc);
		out.write(junk.data(), junk.size());
		out.write(data.data(), data.size());
		return static_cast<bool>(out);
	}

	/// Basic minification of JASS or Lua
	export std::string obfuscate_script(const std::string_view text, const bool lua) {
		std::string out;
		out.reserve(text.size());

		for (const auto line : std::views::split(text, '\n')) {
			std::string_view trimmed(line.data(), line.size());

			constexpr std::string_view whitespace = " \t\r\f\v";
			const size_t begin = trimmed.find_first_not_of(whitespace);
			if (begin == std::string_view::npos) {
				continue; // blank line
			}
			const size_t end = trimmed.find_last_not_of(whitespace);
			trimmed = trimmed.substr(begin, end - begin + 1);

			if (!lua && trimmed.starts_with("//")) {
				continue; // JASS comment-only line
			}
			// Skip Lua line comments, but never a block-comment opener (--[[ / --[==[)
			// since that would span and break following lines.
			if (lua && trimmed.starts_with("--") && !trimmed.substr(2).starts_with("[")) {
				continue;
			}

			out.append(trimmed);
			out.push_back('\n');
		}

		return out;
	}
} // namespace protection
