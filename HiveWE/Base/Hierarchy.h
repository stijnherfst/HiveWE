#pragma once

class Hierarchy {
public:
	char tileset = 'L';
	casc::CASC game_data;
	json::JSON aliases;

	fs::path map_directory;
	fs::path warcraft_directory;

	void open_casc(fs::path directory);

	BinaryReader open_file(const fs::path& path) const;
	bool file_exists(const fs::path& path) const;

	BinaryReader map_file_read(const fs::path& path) const;
	/// Will create the file if it does not exist or overwrite existing
	void map_file_write(const fs::path& path, const std::vector<uint8_t>& data) const;
	void map_file_remove(const fs::path& path) const;
	bool map_file_exists(const fs::path& path) const;
	void map_file_rename(const fs::path& original, const fs::path& renamed) const;
};

extern Hierarchy hierarchy;