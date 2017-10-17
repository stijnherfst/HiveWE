#include "stdafx.h"

void Map::load(std::wstring path) {
	mpq::MPQ mpq(L"Data/t2.w3x");

	mpq::File file = mpq.file_open("war3map.w3e");

	BinaryReader reader(file.read());

	file.close();
	mpq.close();

	std::string magic_number = reader.readString(4);
	uint32_t version = reader.read<uint32_t>(); // 11

	char tileset = reader.read<char>();
	uint32_t customTileset = reader.read<uint32_t>(); // 0 for not custom, 1 for custom

	uint32_t tilesetTextures = reader.read<uint32_t>();
	if (tilesetTextures > 16) {
		// Error invalid, maximum is 16
		std::cout << "More than 16 textures" << std::endl;
	}

	std::vector<std::string> tilesetIDs;// = reader.readVector<uint8_t>(4 * tilesetTextures);
	for (size_t i = 0; i < tilesetTextures; i++) {
		tilesetIDs.push_back(reader.readString(4));
	}
	
	int cliffsetTextures = reader.read<uint32_t>();
	std::vector<uint8_t> cliffsetIDs = reader.readVector<uint8_t>(4 * cliffsetTextures);

	uint32_t width = reader.read<uint32_t>();
	uint32_t height = reader.read<uint32_t>();

	float offsetX = reader.read<float>();
	float offsetY = reader.read<float>();

	struct tilePoint {
		int height = 0;
		int water_level = 0;
		char groundTextureType;
		char textureDetails;
	};

	std::vector<tilePoint> tiles(width * height);

	for (size_t j = 0; j < height; j++) {
		for (size_t i = 0; i < width; i++) {
			tiles[j * width + i].height = reader.read<int16_t>();
			tiles[j * width + i].water_level = reader.read<int16_t>();
			tiles[j * width + i].groundTextureType = reader.read<int8_t>() & 0x0F;
			reader.position += 2;
		}
	}

	terrain.width = width;
	terrain.height = height;
	terrain.corners.resize(width * height);

 	for (size_t i = 0; i < width; i++) {
		for (size_t j = 0; j < height; j++) {
			terrain.corners[j * width + i].texture = tiles[j * width + i].groundTextureType;
			terrain.corners[j * width + i].height = tiles[j * width + i].height;
			//terrain.corners[j * width + i].cliffType = tiles[j * width + i].textureDetails;
		}
	}

	hierarchy.init(tileset);




	slk::SLK slk("TerrainArt\\Terrain.slk");




	for (auto&& i : tilesetIDs) {
		//hierarchy.open_file()
		terrain.textureExtended.push_back(false);

	}
	for (size_t i = 0; i < tilesetIDs.size(); i++) {
		for (size_t j = 0; j < slk.data[0].size(); j++) {
			if (slk.data[0][j] == tilesetIDs[i]) {
				terrain.texturess.push_back(resource_manager.load<Texture>(slk.data[2][j] + "\\" + slk.data[3][j] + ".blp"));
			}
		}
	}

	terrain.create();

	
	//auto dirt = resource_manager.load<Texture>("Data/Images/Village_Dirt.png");
	//auto cobble = resource_manager.load<Texture>("Data/Images/Village_CobblePath.png");
	//auto grass = resource_manager.load<Texture>("TerrainArt\\Ashenvale\\Ashen_Grass.blp");

	//gl->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, grass.get()->width, grass.get()->height, 1, GL_RGBA, GL_UNSIGNED_BYTE, grass.get()->data);
	//gl->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, cobble.get()->width, cobble.get()->height, 1, GL_RGBA, GL_UNSIGNED_BYTE, cobble.get()->data);
	
}