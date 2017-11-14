#pragma once

class StaticMesh : public Resource {
public:

	GLuint vertexBuffer;
	GLuint uvBuffer;
	GLuint normalBuffer;
	GLuint indexBuffer;

	int vertices;
	int indices;

	GLuint texture;

	StaticMesh(const std::string& path);

	void render();
};