#include "stdafx.h"

void Map::load(std::wstring path) {
	mpq::MPQ mpq(L"Data/t3.w3x");

	mpq::File file = mpq.file_open("war3map.w3e");

	std::vector<uint8_t> bytes = file.read();

	file.close();
	mpq.close();

	// Very very very dirty below
	size_t address = 4;
	int version = *(reinterpret_cast<int*>(&bytes[address])); // 11
	address += 4;

	char tileset = static_cast<char>(bytes[address++]);
	int customTileset = *(reinterpret_cast<int*>(&bytes[address])); // 0 for not custom, 1 for custom
	address += 4;

	int tilesetTextures = *(reinterpret_cast<int*>(&bytes[address]));
	address += 4;
	if (tilesetTextures > 16) {
		// Error invalid, maximum is 16
		std::cout << "More than 16 textures" << std::endl;
	}

	std::vector<uint8_t> tilesetIDs(&bytes[address], &bytes[address] + 4 * tilesetTextures);
	address += 4 * tilesetTextures;
	
	int cliffsetTextures = *(reinterpret_cast<int*>(&bytes[address]));
	address += 4;

	std::vector<uint8_t> cliffsetIDs(&bytes[address], &bytes[address] + 4 * cliffsetTextures);
	address += 4 * cliffsetTextures;

	int width = *(reinterpret_cast<int*>(&bytes[address]));
	address += 4;
	int height = *(reinterpret_cast<int*>(&bytes[address]));
	address += 4;

	float offsetX = *(reinterpret_cast<float*>(&bytes[address]));
	address += 4;
	float offsetY = *(reinterpret_cast<float*>(&bytes[address]));
	address += 4;

	struct tilePoint {
		int height = 0;
		char groundTextureType;
		char textureDetails;
	};

	std::vector<tilePoint> tiles(width * height);

	for (size_t j = 0; j < height; j++) {
		for (size_t i = 0; i < width; i++) {
			tiles[j * width + i].height = *(reinterpret_cast<short*>(&bytes[address]));
			address += 7; // Fuck water level
			//uint8_t t = *(reinterpret_cast<char*>(&bytes[address]));

			//tiles[j * width + i].groundTextureType = (*(reinterpret_cast<char*>(&bytes[address]))) << 4;
			//address += 1;
			//tiles[j * width + i].textureDetails = t & 0x0F;
			//address += 2;
		}
	}

	terrain.width = width;
	terrain.height = height;
	terrain.corners.resize(width * height);

 	for (size_t i = 0; i < width; i++) {
		for (size_t j = 0; j < height; j++) {
			terrain.corners[j * width + i].texture = 0;
			terrain.corners[j * width + i].height = tiles[j * width + i].height;
			//terrain.corners[j * width + i].cliffType = tiles[j * width + i].textureDetails;
		}
	}

	hierarchy.init(tileset);
	for (auto&& i : tilesetIDs) {
		//hierarchy.open_file()
		terrain.textureExtended.push_back(false);

	}

	terrain.create();
}