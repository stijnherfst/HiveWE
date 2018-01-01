#include "stdafx.h"

void Map::load(std::wstring path) {
	mpq::MPQ mpq(path);
	BinaryReader war3map_w3e = BinaryReader(mpq.file_open("war3map.w3e").read());
	bool success = terrain.load(war3map_w3e);
	if (!success) {
		return;
	}

	BinaryReader war3map_wpm = BinaryReader(mpq.file_open("war3map.wpm").read());
	success = pathing_map.load(war3map_wpm, terrain);
	if (!success) {
		return;
	}

	BinaryReader war3map_doo = BinaryReader(mpq.file_open("war3map.doo").read());
	success = doodads.load(war3map_doo, terrain);
}