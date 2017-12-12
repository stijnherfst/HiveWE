#include "stdafx.h"

void Map::load(std::wstring path) {
	mpq::MPQ mpq(L"Data/t2.w3x");
	mpq::File war3map_w3e = mpq.file_open("war3map.w3e");
	bool success = terrain.load(war3map_w3e.read());
	if (!success) {
		return;
	}

	mpq::File war3map_wpm = mpq.file_open("war3map.wpm");
	success = pathing_map.load(BinaryReader(war3map_wpm.read()), terrain);
	if (!success) {
		return;
	}
}