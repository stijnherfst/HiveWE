module;

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glad/glad.h>

#include <filesystem>

export module CliffMesh;

import BinaryReader;
import ResourceManager;
import Hierarchy;
import MDX;

namespace fs = std::filesystem;

export class CliffMesh : public Resource {
  public:
	GLuint vertex_buffer;
	GLuint uv_buffer;
	GLuint normal_buffer;
	GLuint index_buffer;
	GLuint instance_buffer;
	size_t indices;

	static constexpr const char* name = "CliffMesh";

	std::vector<glm::vec4> render_jobs;

	explicit CliffMesh(const fs::path& path) {
		if (path.extension() == ".mdx" || path.extension() == ".MDX") {
			auto reader = BinaryReader(hierarchy.open_file(path));
			mdx::MDX model = mdx::MDX(reader);

			auto set = model.geosets.front();

			glCreateBuffers(1, &vertex_buffer);
			glNamedBufferData(vertex_buffer, static_cast<int>(set.vertices.size() * sizeof(glm::vec3)), set.vertices.data(), GL_STATIC_DRAW);

			glCreateBuffers(1, &uv_buffer);
			glNamedBufferData(uv_buffer, static_cast<int>(set.texture_coordinate_sets.front().size() * sizeof(glm::vec2)), set.texture_coordinate_sets.front().data(), GL_STATIC_DRAW);

			glCreateBuffers(1, &normal_buffer);
			glNamedBufferData(normal_buffer, static_cast<int>(set.normals.size() * sizeof(glm::vec3)), set.normals.data(), GL_STATIC_DRAW);

			glCreateBuffers(1, &instance_buffer);

			indices = set.faces.size();
			glCreateBuffers(1, &index_buffer);
			glNamedBufferData(index_buffer, static_cast<int>(set.faces.size() * sizeof(uint16_t)), set.faces.data(), GL_STATIC_DRAW);
		}
	}

	~CliffMesh() {
		glDeleteBuffers(1, &vertex_buffer);
		glDeleteBuffers(1, &uv_buffer);
		glDeleteBuffers(1, &normal_buffer);
		glDeleteBuffers(1, &instance_buffer);
		glDeleteBuffers(1, &index_buffer);
	}

	void render_queue(const glm::vec4 position) {
		render_jobs.push_back(position);
	}

	void render() {
		if (render_jobs.empty()) {
			return;
		}

		glNamedBufferData(instance_buffer, static_cast<int>(render_jobs.size() * sizeof(glm::vec4)), render_jobs.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glEnableVertexAttribArray(3);
		glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
		glVertexAttribDivisor(3, 1);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
		glDrawElementsInstanced(GL_TRIANGLES, indices, GL_UNSIGNED_SHORT, nullptr, static_cast<int>(render_jobs.size()));

		glVertexAttribDivisor(3, 0); // ToDo use vao
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);

		render_jobs.clear();
	}
};