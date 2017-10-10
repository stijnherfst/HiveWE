#include "stdafx.h"

Hierarchy hierarchy;
const std::wstring warcraftDirectory = L"C:/Program Files (x86)/Warcraft III/";

Hierarchy::~Hierarchy() {
	tileset.close();
	war3Patch.close();
	war3xLocal.close();
	war3x.close();
	war3Local.close();
	war3.close();
}

bool Hierarchy::init(char tileset_code) {
	war3Patch.open(warcraftDirectory + L"War3Patch.mpq");
	war3xLocal.open(warcraftDirectory + L"War3xlocal.mpq");
	war3x.open(warcraftDirectory + L"War3x.mpq");
	war3Local.open(warcraftDirectory + L"War3local.mpq");
	war3.open(warcraftDirectory + L"War3.mpq");

	std::string file_name = tileset_code + ".mpq"s;
	
	mpq::File tileset_mpq;
	if (war3Patch.file_exists(file_name)) {
		tileset_mpq = war3Patch.file_open(file_name);
	} else if (war3xLocal.file_exists(file_name)) {
		tileset_mpq = war3xLocal.file_open(file_name);
	} else if(war3x.file_exists(file_name)) {
		tileset_mpq = war3x.file_open(file_name);
	} else if (war3Local.file_exists(file_name)) {
		tileset_mpq = war3Local.file_open(file_name);
	} else if (war3.file_exists(file_name)) {
		tileset_mpq = war3.file_open(file_name);
	} else {
		return false;
	}

	tileset.open(tileset_mpq);

	return true;
}

std::vector<uint8_t> Hierarchy::open_file(std::string path) {
	mpq::File file;
	if (tileset.file_exists(path)) {
		file = tileset.file_open(path);
	} else if (war3Patch.file_exists(path)) {
		file = war3Patch.file_open(path);
	} else if (war3xLocal.file_exists(path)) {
		file = war3xLocal.file_open(path);
	} else if (war3x.file_exists(path)) {
		file = war3x.file_open(path);
	} else if (war3Local.file_exists(path)) {
		file = war3Local.file_open(path);
	} else if (war3.file_exists(path)) {
		file = war3.file_open(path);
	} else {
		//return false;
		std::cout << "Unable to find file in hierarchy";
		return std::vector<uint8_t>();
	}

	return file.read();
}