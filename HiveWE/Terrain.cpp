#include "stdafx.h"

glm::vec2 Terrain::getTileVariation(unsigned char variation, bool extended) {
	if (extended) {
		if (variation <= 15) {
			return glm::vec2(4 + (variation % 4), variation / 4);
		} else if (variation == 16) {
			return glm::vec2(3, 3);
		} else {
			return glm::vec2(0, 0);
		}
	} else {
		if (variation == 0) {
			return glm::vec2(0, 0);
		} else {
			return glm::vec2(3, 3);
		}
	}
}

void Terrain::create() {
	vertices.resize(4 * 4 * width * height);
	uvs.resize(4 * 4 * width * height);
	indices.resize((width - 1) * (height - 1) * 4 * 2);
	corners.resize(width * height);

	corners[0].texture = 0;
	corners[1].texture = 0;
	corners[2].texture = 0;
	corners[3].texture = 0;
	corners[4].texture = 0;

	corners[5].texture = 0;
	corners[6].texture = 0;
	corners[7].texture = 0;
	corners[8].texture = 0;
	corners[9].texture = 0;

	corners[10].texture = 0;
	corners[11].texture = 1;
	corners[12].texture = 1;
	corners[13].texture = 1;
	corners[14].texture = 0;

	corners[15].texture = 0;
	corners[16].texture = 0;
	corners[17].texture = 0;
	corners[18].texture = 0;
	corners[19].texture = 0;

	corners[20].texture = 0;
	corners[21].texture = 0;
	corners[22].texture = 0;
	corners[23].texture = 0;
	corners[24].texture = 0;

	unsigned int it = 0;
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			unsigned int index = (j * width + i) * 4 * 4;
			unsigned char texture = corners[j * width + i].texture;

			// Top right
			if (i < width - 1 && j < height - 1) {
				vertices[index + 0] = { i + 1, j + 1, 0 };
				vertices[index + 1] = { i, j + 1, 0 };
				vertices[index + 2] = { i, j, 0 };
				vertices[index + 3] = { i + 1, j, 0 };

				uvs[index + 0] = { 0.125, 0.125, texture };
				uvs[index + 1] = { 0, 0.125, texture };
				uvs[index + 2] = { 0, 0, texture };
				uvs[index + 3] = { 0.125, 0, texture };

				indices[it++] = { index + 0, index + 1, index + 2 };
				indices[it++] = { index + 0, index + 2, index + 3 };
			}

			// Top left
			if (i > 0 && j < height - 1) {
				vertices[index + 4] = { i, j + 1, 1 };
				vertices[index + 5] = { i - 1, j + 1, 1 };
				vertices[index + 6] = { i - 1, j, 1 };
				vertices[index + 7] = { i, j, 1 };

				uvs[index + 4] = { 0.125, 0.125, texture };
				uvs[index + 5] = { 0, 0.125, texture };
				uvs[index + 6] = { 0, 0, texture };
				uvs[index + 7] = { 0.125, 0, texture };

				indices[it++] = { index + 4, index + 5, index + 6 };
				indices[it++] = { index + 4, index + 6, index + 7 };
			}

			// Bottom left
			if (i > 0 && j > 0) {
				vertices[index + 8] = { i, j, 2 };
				vertices[index + 9] = { i - 1, j, 2 };
				vertices[index + 10] = { i - 1, j - 1, 2 };
				vertices[index + 11] = { i, j - 1, 2 };

				uvs[index + 8] = { 0.125, 0.125, texture };
				uvs[index + 9] = { 0, 0.125, texture };
				uvs[index + 10] = { 0, 0, texture };
				uvs[index + 11] = { 0.125, 0, texture };

				indices[it++] = { index + 8, index + 9, index + 10 };
				indices[it++] = { index + 8, index + 10, index + 11 };
			}

			// Bottom right
			if (i < width - 1 && j > 0) {
				vertices[index + 12] = { i + 1, j, 3 };
				vertices[index + 13] = { i, j, 3 };
				vertices[index + 14] = { i, j - 1, 3 };
				vertices[index + 15] = { i + 1, j - 1, 3 };

				uvs[index + 12] = { 0.125, 0.125, texture };
				uvs[index + 13] = { 0, 0.125, texture };
				uvs[index + 14] = { 0, 0, texture };
				uvs[index + 15] = { 0.125, 0, texture };

				indices[it++] = { index + 12, index + 13, index + 14 };
				indices[it++] = { index + 12, index + 14, index + 15 };
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
	unsigned char* image = SOIL_load_image("Data/Images/Village_CobblePath.png", &imgWidth, &imgHeight, &imgChannels, SOIL_LOAD_AUTO);
	gl->glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, imgWidth, imgHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE, image);

	unsigned char* image2 = SOIL_load_image("Data/Images/Village_Dirt.png", &imgWidth, &imgHeight, &imgChannels, SOIL_LOAD_AUTO);
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
	
	std::cout << "2 " << gl->glGetError() << std::endl;
}