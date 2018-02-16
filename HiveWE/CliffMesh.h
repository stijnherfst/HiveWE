#pragma once

class CliffMesh : public Resource {
public:
	GLuint vertexBuffer;
	GLuint uvBuffer;
	GLuint normalBuffer;
	GLuint indexBuffer;
	GLuint instanceBuffer;

	int vertices;
	int indices;

	static constexpr const char* name = "CliffMesh";

	std::vector<glm::vec4> render_jobs;

	CliffMesh(const fs::path& path);
	virtual ~CliffMesh();

	void render_queue(glm::vec4 position);

	void render();
};