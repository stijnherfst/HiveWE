#include "stdafx.h"

bool PathingMap::load(BinaryReader& reader) {
	std::string magic_number = reader.readString(4);
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

	pathing_map = reader.readVector<uint8_t>(width * height);
}

void PathingMap::create() {
	//vertices.reserve(width * height * 4 * 1.5);
	//uvs.reserve(width * height * 4 * 1.5);
	//indices.reserve((width - 1) * (height - 1) * 2 * 1.5);

	//for (int i = 0; i < width - 1; i++) {
	//	for (int j = 0; j < height - 1; j++) {
	//		// Water
	//		if (bottomLeft.water || bottomRight.water || topLeft.water || topRight.water) {
	//			water_vertices.push_back({ i + 1,	j + 1,	corner_water_height(bottomLeft) });
	//			water_vertices.push_back({ i,		j + 1,	corner_water_height(bottomLeft) });
	//			water_vertices.push_back({ i,		j,		corner_water_height(bottomLeft) });
	//			water_vertices.push_back({ i + 1,	j,		corner_water_height(bottomLeft) });

	//			water_uvs.push_back({ 1, 1 });
	//			water_uvs.push_back({ 0, 1 });
	//			water_uvs.push_back({ 0, 0 });
	//			water_uvs.push_back({ 1, 0 });

	//			// Calculate water colour based on distance to the terrain
	//			glm::vec4 color;
	//			for (auto&& corner : { topRight, topLeft, bottomLeft, bottomRight }) {
	//				float value = std::clamp(corner_water_height(corner) - corner_height(corner), 0.f, 1.f);
	//				if (value <= deeplevel) {
	//					value = std::max(0.f, value - min_depth) / (deeplevel - min_depth);
	//					color = shallow_color_min * (1.f - value) + shallow_color_max * value;
	//				} else {
	//					value = std::clamp(value - deeplevel, 0.f, maxdepth - deeplevel) / (maxdepth - deeplevel);
	//					color = deep_color_min * (1.f - value) + deep_color_max * value;
	//				}
	//				water_colors.push_back(color / glm::vec4(255, 255, 255, 255));
	//			}

	//			unsigned int index = water_vertices.size() - 4;
	//			water_indices.push_back({ index + 0, index + 3, index + 1 });
	//			water_indices.push_back({ index + 1, index + 3, index + 2 });
	//		}
	//	}
	//}

	//// Ground
	//gl->glCreateBuffers(1, &vertex_buffer);
	//gl->glNamedBufferData(vertex_buffer, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	//gl->glCreateBuffers(1, &uv_buffer);
	//gl->glNamedBufferData(uv_buffer, uvs.size() * sizeof(glm::vec3), uvs.data(), GL_STATIC_DRAW);

	//gl->glCreateBuffers(1, &index_buffer);
	//gl->glNamedBufferData(index_buffer, indices.size() * sizeof(unsigned int) * 3, indices.data(), GL_STATIC_DRAW);

	//// Water
	//gl->glCreateBuffers(1, &water_vertex_buffer);
	//gl->glNamedBufferData(water_vertex_buffer, water_vertices.size() * sizeof(glm::vec3), water_vertices.data(), GL_STATIC_DRAW);

	//gl->glCreateBuffers(1, &water_uv_buffer);
	//gl->glNamedBufferData(water_uv_buffer, water_uvs.size() * sizeof(glm::vec2), water_uvs.data(), GL_STATIC_DRAW);

	//gl->glCreateBuffers(1, &water_color_buffer);
	//gl->glNamedBufferData(water_color_buffer, water_colors.size() * sizeof(glm::vec4), water_colors.data(), GL_STATIC_DRAW);

	//gl->glCreateBuffers(1, &water_index_buffer);
	//gl->glNamedBufferData(water_index_buffer, water_indices.size() * sizeof(unsigned int) * 3, water_indices.data(), GL_STATIC_DRAW);
}