#include "stdafx.h"

//glm::vec2 Terrain::getTileVariation(unsigned char variation, bool extended) {
//	if (extended) {
//		if (variation <= 15) {
//			return glm::vec2(4 + (variation % 4), variation / 4);
//		} else if (variation == 16) {
//			return glm::vec2(3, 3);
//		} else {
//			return glm::vec2(0, 0);
//		}
//	} else {
//		if (variation == 0) {
//			return glm::vec2(0, 0);
//		} else {
//			return glm::vec2(3, 3);
//		}
//	}
//}

// TODO extended textures
std::vector<std::tuple<int, int, int>> Terrain::getCornerVariation(unsigned char topLeft, unsigned char topRight, unsigned char bottomLeft, unsigned char bottomRight) {
	std::vector<std::tuple<int, int, int>> tileVariations;

	std::bitset<4> var;
	for (auto&& texture : std::set<int>({ topLeft, topRight, bottomLeft, bottomRight })) {
		var[0] = bottomRight == texture;
		var[1] = bottomLeft == texture;
		var[2] = topRight == texture;
		var[3] = topLeft == texture;

		tileVariations.push_back({ var.to_ulong() % 4, std::floor(var.to_ulong() / 4), texture });
	}
	return tileVariations;
}

void Terrain::create() {
	vertices.reserve(width * height * 4);
	uvs.reserve(width * height * 4);
	indices.reserve((width - 1) * (height - 1) * 2);

	corners.resize(width * height);
	corners[0].texture = 0; 
	corners[1].texture = 0;
	corners[2].texture = 0;

	corners[3].texture = 1;
	corners[4].texture = 1;
	corners[5].texture = 0;

	corners[6].texture = 0;
	corners[7].texture = 0;
	corners[8].texture = 0;

	// Becomes
	// 000
	// 110
	// 000

	for (int i = 0; i < width - 1; i++) {
		for (int j = 0; j < height - 1; j++) {
			unsigned char bottomLeft = corners[j * width + i].texture;
			unsigned char bottomRight = corners[j * width + (i + 1)].texture;
			unsigned char topLeft = corners[(j + 1) * width + i].texture;
			unsigned char topRight = corners[(j + 1) * width + (i + 1)].texture;

			auto variations = getCornerVariation(bottomLeft, bottomRight, topLeft, topRight);

			for (auto&& [x, y, texture] : variations) {
				vertices.push_back({ i + 1, j + 1,	0 });
				vertices.push_back({ i, j + 1,		0 });
				vertices.push_back({ i, j,			0 });
				vertices.push_back({ i + 1, j,		0 });

				uvs.push_back({ 0.125 * (x + 1), 0.125 * (y + 1),	texture });
				uvs.push_back({ 0.125 * x, 0.125 * (y + 1),			texture });
				uvs.push_back({ 0.125 * x, 0.125 * y,				texture });
				uvs.push_back({ 0.125 * (x + 1), 0.125 * y,			texture });

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
	gl->glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, textureWidth, textureHeight, 2);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	int imgWidth, imgHeight, imgChannels;
	unsigned char* image = SOIL_load_image("Data/Images/Village_Dirt.png", &imgWidth, &imgHeight, &imgChannels, SOIL_LOAD_AUTO);
	gl->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, imgWidth, imgHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE, image);

	unsigned char* image2 = SOIL_load_image("Data/Images/Village_CobblePath.png", &imgWidth, &imgHeight, &imgChannels, SOIL_LOAD_AUTO);
	gl->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, imgWidth, imgHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE, image2);

	gl->glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	std::cout << gl->glGetError() << std::endl;
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