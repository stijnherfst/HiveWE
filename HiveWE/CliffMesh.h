#pragma once

class CliffMesh : public Resource {
public:
	GLuint vertexBuffer;
	GLuint uvBuffer;
	GLuint normalBuffer;
	GLuint indexBuffer;

	int vertices;
	int indices;

	std::shared_ptr<GPUTexture> texture;

	static constexpr const char* name = "CliffMesh";

	CliffMesh(const std::string& path);
	virtual ~CliffMesh();

	void render();
};