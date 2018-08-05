#pragma once

class Hierarchy {
public:
	mpq::MPQ map;
	mpq::MPQ tileset;
	mpq::MPQ war3Mod;
	mpq::MPQ war3xLocal;
	mpq::MPQ war3x;
	mpq::MPQ war3Local;
	mpq::MPQ war3;
	mpq::MPQ deprecated;

	fs::path warcraft_directory;

	void init();
	void load_tileset(char tileset_code);

	BinaryReader open_file(const fs::path& path) const;
	bool file_exists(const fs::path& path) const;
};

extern Hierarchy hierarchy;