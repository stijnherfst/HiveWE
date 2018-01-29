#pragma once

class Hierarchy {
public:
	mpq::MPQ tileset;
	mpq::MPQ map;
	mpq::MPQ war3Patch;
	mpq::MPQ war3xLocal;
	mpq::MPQ war3x;
	mpq::MPQ war3Local;
	mpq::MPQ war3;

	fs::path warcraft_directory;
	
	bool init(char tileset_code);
	BinaryReader open_file(std::string path);
};

extern Hierarchy hierarchy;