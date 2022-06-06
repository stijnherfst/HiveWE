#pragma once

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <QOpenGLFunctions_4_5_Core>
#include <filesystem>
namespace fs = std::filesystem;

import ResourceManager;

class CliffMesh : public Resource {
public:
	GLuint vertex_buffer;
	GLuint uv_buffer;
	GLuint normal_buffer;
	GLuint index_buffer;
	GLuint instance_buffer;
	size_t indices;

	static constexpr const char* name = "CliffMesh";

	std::vector<glm::vec4> render_jobs;

	explicit CliffMesh(const fs::path& path);
	virtual ~CliffMesh();

	void render_queue(glm::vec4 position);

	void render();
};