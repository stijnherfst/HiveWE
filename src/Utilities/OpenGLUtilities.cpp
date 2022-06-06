#include "OpenGLUtilities.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <QSettings>
#include <QPainter>
#include <QIcon>
#include <QOpenGLFunctions_4_5_Core>

#include <filesystem>
namespace fs = std::filesystem;

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <fmt/format.h>

import Texture;
import ResourceManager;
import Utilities;

QOpenGLFunctions_4_5_Core* gl;
Shapes shapes;

void Shapes::init() {
	const std::vector<glm::vec2> quad_vertices = {
		{ 1, 1 },
		{ 0, 1 },
		{ 0, 0 },
		{ 1, 0 }
	};

	const std::vector<glm::uvec3> quad_indices = {
		{ 0, 1, 2 },
		{ 2, 3, 0 }
	};

	gl->glCreateBuffers(1, &vertex_buffer);
	gl->glNamedBufferData(vertex_buffer, quad_vertices.size() * sizeof(glm::vec2), quad_vertices.data(), GL_STATIC_DRAW);

	gl->glCreateBuffers(1, &index_buffer);
	gl->glNamedBufferData(index_buffer, quad_indices.size() * sizeof(unsigned int) * 3, quad_indices.data(), GL_STATIC_DRAW);
}

GLuint compile_shader(const fs::path& vertex_shader, const fs::path& fragment_shader) {
	char buffer[512];
	GLint status;

	std::string vertex_source = read_text_file(vertex_shader.string());
	std::string fragment_source = read_text_file(fragment_shader.string());

	const GLuint vertex = gl->glCreateShader(GL_VERTEX_SHADER);
	const GLuint fragment = gl->glCreateShader(GL_FRAGMENT_SHADER);

	// Vertex Shader
	const char* source = vertex_source.c_str();
	gl->glShaderSource(vertex, 1, &source, nullptr);
	gl->glCompileShader(vertex);

	gl->glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);
	if (!status) {
		gl->glGetShaderInfoLog(vertex, 512, nullptr, buffer);
		fmt::print("{}\n{}\n", vertex_shader.string(), buffer);
		return -1;
	}

	// Fragment Shader
	source = fragment_source.c_str();
	gl->glShaderSource(fragment, 1, &source, nullptr);
	gl->glCompileShader(fragment);

	gl->glGetShaderiv(fragment, GL_COMPILE_STATUS, &status);
	if (!status) {
		gl->glGetShaderInfoLog(fragment, 512, nullptr, buffer);
		fmt::print("{}\n{}\n", fragment_shader.string(), buffer);
		return -1;
	}

	// Link
	const GLuint shader = gl->glCreateProgram();
	gl->glAttachShader(shader, vertex);
	gl->glAttachShader(shader, fragment);
	gl->glLinkProgram(shader);

	gl->glGetProgramiv(shader, GL_LINK_STATUS, &status);
	if (!status) {
		gl->glGetProgramInfoLog(shader, 512, nullptr, buffer);
		fmt::print("{}\n{}\n{}\n", vertex_shader.string(), fragment_shader.string(), buffer);
		return -1;
	}

	gl->glDeleteShader(vertex);
	gl->glDeleteShader(fragment);

	return shader;
}

QIcon ground_texture_to_icon(uint8_t* data, const int width, const int height) {
	QImage temp_image = QImage(data, width, height, QImage::Format::Format_RGBA8888);
	const int size = height / 4;

	auto pix = QPixmap::fromImage(temp_image.copy(0, 0, size, size));

	QIcon icon;
	icon.addPixmap(pix, QIcon::Normal, QIcon::Off);

	QPainter painter(&pix);
	painter.fillRect(0, 0, size, size, QColor(255, 255, 0, 64));
	painter.end();

	icon.addPixmap(pix, QIcon::Normal, QIcon::On);

	return icon;
}

QIcon texture_to_icon(fs::path path) {
	auto tex = resource_manager.load<Texture>(path);
	QImage temp_image = QImage(tex->data.data(), tex->width, tex->height, tex->channels == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_RGBA8888);
	auto pix = QPixmap::fromImage(temp_image);
	return QIcon(pix);
};

fs::path find_warcraft_directory() {
	QSettings settings;
	if (settings.contains("warcraftDirectory")) {
		return settings.value("warcraftDirectory").toString().toStdString();
	} else if (fs::exists("C:/Program Files/Warcraft III")) {
		return "C:/Program Files/Warcraft III";
	} else if (fs::exists("C:/Program Files (x86)/Warcraft III")) {
		return "C:/Program Files (x86)/Warcraft III";
	} else {
		return "";
	}
}