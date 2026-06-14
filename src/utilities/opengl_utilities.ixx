module;

#include <QPainter>
#include <QIcon>

export module OpenGLUtilities;

import std;
import ResourceManager;
import Texture;
import <glad/glad.h>;
import <glm/glm.hpp>;

namespace fs = std::filesystem;

export class Shapes {
  public:
	void init() {
		glCreateBuffers(1, &vertex_buffer);
		glNamedBufferData(vertex_buffer, quad_vertices.size() * sizeof(glm::vec2), quad_vertices.data(), GL_STATIC_DRAW);

		glCreateBuffers(1, &index_buffer);
		glNamedBufferData(index_buffer, quad_indices.size() * sizeof(unsigned int) * 3, quad_indices.data(), GL_STATIC_DRAW);
	}

	GLuint vertex_buffer;
	GLuint index_buffer;

	const std::vector<glm::vec2> quad_vertices = {{1, 1}, {0, 1}, {0, 0}, {1, 0}};

	const std::vector<glm::uvec3> quad_indices = {{0, 1, 2}, {2, 3, 0}};
};

export inline Shapes shapes;

/// Loads a texture from the hierarchy and returns an icon
export QIcon texture_to_icon(const fs::path& path) {
	const auto tex = resource_manager.load<Texture>(path).value();
	const QImage temp_image = QImage(
		tex->data.data(),
		tex->width,
		tex->height,
		tex->channels == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_RGBA8888
	);
	const auto pix = QPixmap::fromImage(temp_image);
	return QIcon(pix);
}
