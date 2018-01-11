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
	gl->glTextureSubImage2D(pathing_texture, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, pathing_cells.data());
	gl->glTextureParameteri(pathing_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	gl->glTextureParameteri(pathing_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl->glTextureParameteri(pathing_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTextureParameteri(pathing_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


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
	//vertices.resize(width * height * 4);
	//colors.resize(width * height * 4);
	//indices.resize((width - 1) * (height - 1) * 2);

	//auto mix = [](float min, float max, float value) {
	//	return min * value + max * (1.f - value); 
	//};

	/*for (int i = 0; i < width - 1; i++) {
		for (int j = 0; j < height - 1; j++) {
			if (pathing_map[j * width + i].walkable && pathing_map[j * width + i].flyable && pathing_map[j * width + i].buildable) {
				continue;
			}

			int x = std::floor(i * 0.25);
			int y = std::floor(j * 0.25);

			float botLeft = terrain.corner_height(x, y);
			float botRight = terrain.corner_height(x + 1, y);
			float topLeft = terrain.corner_height(x, y + 1);
			float topRight = terrain.corner_height(x + 1, y + 1);

			auto terrain_height = [&](float i, float j) {
				float z = mix(botRight, botLeft, i - x);
				float zz = mix(topRight, topLeft, i - x);

				return mix(zz, z, j - y) + 0.01;
			};

			vertices.push_back({ i * 0.25 + 0.25,	j * 0.25 + 0.25,	terrain_height(i * 0.25 + 0.25,	j * 0.25 + 0.25) });
			vertices.push_back({ i * 0.25,			j * 0.25 + 0.25,	terrain_height(i * 0.25,		j * 0.25 + 0.25) });
			vertices.push_back({ i * 0.25,			j * 0.25,			terrain_height(i * 0.25,		j * 0.25) });
			vertices.push_back({ i * 0.25 + 0.25,	j * 0.25,			terrain_height(i * 0.25 + 0.25,	j * 0.25) });

			glm::vec3 color = pathing_map[j * width + i].color();
			colors.push_back(color);
			colors.push_back(color);
			colors.push_back(color);
			colors.push_back(color);

			unsigned int index = vertices.size() - 4;
			indices.push_back({ index + 0, index + 3, index + 1 });
			indices.push_back({ index + 1, index + 3, index + 2 });
		}
	}

	gl->glCreateBuffers(1, &vertex_buffer);
	gl->glNamedBufferData(vertex_buffer, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	gl->glCreateBuffers(1, &color_buffer);
	gl->glNamedBufferData(color_buffer, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);

	gl->glCreateBuffers(1, &index_buffer);
	gl->glNamedBufferData(index_buffer, indices.size() * sizeof(unsigned int) * 3, indices.data(), GL_STATIC_DRAW);*/
}

void PathingMap::render() {
	/*shader->use();

	glm::mat4 Model = glm::mat4(1.0f);
	glm::mat4 MVP = camera.projection * camera.view * Model;

	gl->glUniformMatrix4fv(2, 1, GL_FALSE, &MVP[0][0]);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glEnableVertexAttribArray(1);
	gl->glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
	gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	gl->glDrawElements(GL_TRIANGLES, indices.size() * 3, GL_UNSIGNED_INT, NULL);

	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);*/
}