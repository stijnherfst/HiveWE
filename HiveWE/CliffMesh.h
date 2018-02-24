#pragma once

class CliffMesh : public Resource {
public:
	GLuint vertex_buffer;
	GLuint uv_buffer;
	GLuint normal_buffer;
	GLuint index_buffer;
	GLuint instance_buffer;

	int vertices;
	int indices;

	static constexpr const char* name = "CliffMesh";

	std::vector<glm::vec4> render_jobs;

	explicit CliffMesh(const fs::path& path);
	virtual ~CliffMesh();

	void render_queue(glm::vec4 position);

	void render();
};