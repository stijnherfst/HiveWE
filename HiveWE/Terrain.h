#pragma once

struct index {
	unsigned int first, second, third;
};

class Terrain {
public:
	int width = 5;
	int height = 5;

	int textures = 16;
	int textureWidth = 512;
	int textureHeight = 512;

	GLuint vertexBuffer;
	GLuint uvBuffer;
	GLuint indexBuffer;

	std::vector <glm::vec3> vertices;
	std::vector <glm::vec3> uvs;
	std::vector <index> indices;

	GLuint textureArray;

	void create();
	void render();
};