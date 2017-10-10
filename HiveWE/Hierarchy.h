#pragma once

class Hierarchy {
public:
	mpq::MPQ tileset;
	mpq::MPQ war3Patch;
	mpq::MPQ war3xLocal;
	mpq::MPQ war3x;
	mpq::MPQ war3Local;
	mpq::MPQ war3;

	Hierarchy() {};
	~Hierarchy();
	
	bool init(char tileset_code);
	std::vector<uint8_t> open_file(std::string path);
};

extern Hierarchy hierarchy;