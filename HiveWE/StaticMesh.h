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

	unsigned int shader;
	std::string vertexShader =

		"#version 330 core\n"

		"layout(location = 0) in vec3 vPosition;"

		"uniform mat4 MVP;"

		"void main() {"
		"	gl_Position = MVP * vec4(vPosition, 1);"
		"}";

	std::string fragmentShader =

		"#version 330 core\n"

		"out vec3 color;"

		"void main() {"
		"	color = vec3(1, 1, 1);"
		"};";

	StaticMesh(const std::string& path) : Resource(path) {
		load(path);
	}

	void render();

protected:
	void load(const std::string& path);

	void unload();
};