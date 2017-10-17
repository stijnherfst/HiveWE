#include "stdafx.h"

std::tuple<int, int> Terrain::get_tile_variation(unsigned char variation, bool extended) {
	if (extended) {
		if (variation <= 15) {
			return { 4 + (variation % 4), variation / 4 };
		} else if (variation == 16) {
			return { 3, 3 };
		} else {
			return { 0, 0 };
		}
	} else {
		if (variation == 0) {
			return { 0, 0 };
		} else {
			return { 3, 3 };
		}
	}
}

std::vector<std::tuple<int, int, int>> Terrain::get_texture_variations(Corner& topLeft, Corner& topRight, Corner& bottomLeft, Corner& bottomRight) {
	std::vector<std::tuple<int, int, int>> tileVariations;

	std::bitset<4> index;
	for (auto&& texture : std::set<int>({ topLeft.texture, topRight.texture, bottomLeft.texture, bottomRight.texture })) {
		index[0] = bottomRight.texture == texture;
		index[1] = bottomLeft.texture == texture;
		index[2] = topRight.texture == texture;
		index[3] = topLeft.texture == texture;

		if (index.all()) { // Only bottom left variation matters
			auto [x, y] = get_tile_variation(bottomLeft.variation, textureExtended[texture]);
			tileVariations.push_back({ x, y, texture });
		} else {
			tileVariations.push_back({ index.to_ulong() % 4, (unsigned)std::floor(index.to_ulong() / 4ul), texture });
		}
	}
	return tileVariations;
}

void Terrain::create() {
	vertices.reserve(width * height * 4);
	uvs.reserve(width * height * 4);
	indices.reserve((width - 1) * (height - 1) * 2);

	for (int i = 0; i < width - 1; i++) {
		for (int j = 0; j < height - 1; j++) {
			Corner& bottomLeft = corners[j * width + i];
			Corner& bottomRight = corners[j * width + (i + 1)];
			Corner& topLeft = corners[(j + 1) * width + i];
			Corner& topRight = corners[(j + 1) * width + (i + 1)];

			auto variations = get_texture_variations(bottomLeft, bottomRight, topLeft, topRight); // TODO Bottom and top reversed, fix
			for (auto&& [x, y, texture] : variations) {
				vertices.push_back({ i + 1, j + 1,	(topRight.height - 0x2000) / 512.0 });
				vertices.push_back({ i, j + 1,		(topLeft.height - 0x2000) / 512.0 });
				vertices.push_back({ i, j,			(bottomLeft.height - 0x2000) / 512.0 });
				vertices.push_back({ i + 1, j,		(bottomRight.height - 0x2000) / 512.0 });

				uvs.push_back({ 0.125 * (x + 1), 0.125 * (y + 1), texture });
				uvs.push_back({ 0.125 * x, 0.125 * (y + 1), texture });
				uvs.push_back({ 0.125 * x, 0.125 * y, texture });
				uvs.push_back({ 0.125 * (x + 1), 0.125 * y, texture });

				unsigned int index = vertices.size() - 4;
				indices.push_back({ index + 0, index + 1, index + 2 });
				indices.push_back({ index + 0, index + 2, index + 3 });
			}
		}
	}

	gl->glGenBuffers(1, &vertexBuffer);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	gl->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	gl->glGenBuffers(1, &uvBuffer);
	gl->glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	gl->glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec3), uvs.data(), GL_STATIC_DRAW);

	gl->glGenBuffers(1, &indexBuffer);
	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int) * 3, indices.data(), GL_STATIC_DRAW);

	gl->glGenTextures(1, &textureArray);
	gl->glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
	gl->glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, textureWidth, textureHeight, 13);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	for (size_t i = 0; i < texturess.size(); i++) {

		gl->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, texturess[i].get()->width, texturess[i].get()->height, 1, GL_RGBA, GL_UNSIGNED_BYTE, texturess[i].get()->data);
	}

	gl->glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

void Terrain::render() {
	gl->glActiveTexture(GL_TEXTURE0);
	gl->glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glEnableVertexAttribArray(1);
	gl->glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	gl->glDrawElements(GL_TRIANGLES, indices.size() * 3, GL_UNSIGNED_INT, NULL);

	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);
}