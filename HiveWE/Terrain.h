#pragma once

struct index {
	unsigned int first, second, third;
};

struct corner {
	short height = 0;
	unsigned char texture = 0;
	unsigned char variation = 0;
	unsigned char cliffType = 0;
};

class Terrain {
public:
	int width = 5;
	int height = 5;

	int textures = 16;
	int textureWidth = 512;
	int textureHeight = 512;

	std::vector <corner> corners;

	GLuint vertexBuffer;
	GLuint uvBuffer;
	GLuint indexBuffer;

	std::vector <glm::vec3> vertices;
	std::vector <glm::vec3> uvs;
	std::vector <index> indices;

	GLuint textureArray;

	void create();
	void render();

	glm::vec2 getTileVariation(unsigned char variation, bool extended);
};