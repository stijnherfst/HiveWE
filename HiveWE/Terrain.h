#pragma once

struct Index {
	unsigned int first, second, third;
};

struct Corner {
	short height = 0;
	unsigned char texture = 0;
	unsigned char variation = 0;
	unsigned char cliffType = 0;
};

class Terrain {
public:
	int width = 3;
	int height = 3;

	int textures = 16;
	int textureWidth = 512;
	int textureHeight = 512;

	std::vector <Corner> corners;

	GLuint vertexBuffer;
	GLuint uvBuffer;
	GLuint indexBuffer;

	std::vector <glm::vec3> vertices;
	std::vector <glm::vec3> uvs;
	std::vector <Index> indices;

	GLuint textureArray;

	void create();
	void render();

	glm::vec2 getTileVariation(unsigned char variation, bool extended);
	std::vector<std::tuple<int, int, int>> getCornerVariation(unsigned char topLeft, unsigned char topRight, unsigned char bottomLeft, unsigned char bottomRight);
};