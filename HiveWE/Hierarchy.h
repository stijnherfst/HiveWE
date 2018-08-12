#pragma once

class Hierarchy {
public:
	char tileset = 'L';
	mpq::MPQ map;
	casc::CASC game_data;
	json::JSON aliases;

	fs::path warcraft_directory;

	void init();

	BinaryReader open_file(const fs::path& path) const;
	bool file_exists(const fs::path& path) const;
};

extern Hierarchy hierarchy;