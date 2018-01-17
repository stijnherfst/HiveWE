#include "stdafx.h"

bool PathingMap::load(BinaryReader& reader, Terrain& terrain) {
	std::string magic_number = reader.read_string(4);
	if (magic_number != "MP3W") {
		std::cout << "Invalid war3map.w3e file: Magic number is not W3E!" << std::endl;
		return false;
	}

	int version = reader.read<uint32_t>();
	if (version != 0) {
		std::cout << "Unknown Pathmap version. Attempting to load, but may crash.";
	}

	width = reader.read<uint32_t>();
	height = reader.read<uint32_t>();

	//pathing_map.resize(width * height);
	pathing_cells = reader.read_vector<uint8_t>(width * height);

	gl->glCreateTextures(GL_TEXTURE_2D, 1, &pathing_texture);
	gl->glTextureStorage2D(pathing_texture, 1, GL_R8UI, width, height);
	gl->glTextureSubImage2D(pathing_texture, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing_cells.data());
	gl->glTextureParameteri(pathing_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	gl->glTextureParameteri(pathing_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl->glTextureParameteri(pathing_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTextureParameteri(pathing_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	terrain.pathing_map_texture = pathing_texture;
	//for (int i = 0; i < width * height; i++) {
	//	pathing_map[i].walkable = !(pathing_cells[i] &	0b00000010);
	//	pathing_map[i].flyable = !(pathing_cells[i] &	0b00000100);
	//	pathing_map[i].buildable = !(pathing_cells[i] & 0b00001000);
	//	pathing_map[i].blight = !(pathing_cells[i] &	0b00100000);
	//	pathing_map[i].water = !(pathing_cells[i] &		0b01000000);
	//}

	//shader = resource_manager.load<Shader>({ "Data/Shaders/pathing.vs", "Data/Shaders/pathing.fs" });

	create(terrain);

	return true;
}

// Use half float types to use less memory?
void PathingMap::create(Terrain& terrain) {
	
}

void PathingMap::render() {

}