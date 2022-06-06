#pragma once

#include <vector>
#include <filesystem>

#include <QOpenGLFunctions_4_5_Core>

import BinaryReader;
import BinaryWriter;
#include <SLK.h>

namespace fs = std::filesystem;

constexpr int mod_table_write_version = 2;

class Shapes {
  public:
	void init();

	GLuint vertex_buffer;
	GLuint index_buffer;

	static constexpr int vertex_count = 4;
	static constexpr int index_count = 2;
};

GLuint compile_shader(const fs::path& vertex_shader, const fs::path& fragment_shader);

/// Convert a Tground texture into an QIcon with two states
QIcon ground_texture_to_icon(uint8_t* data, int width, int height);

/// Loads a texture from the hierarchy and returns an icon
QIcon texture_to_icon(fs::path);

fs::path find_warcraft_directory();

extern QOpenGLFunctions_4_5_Core* gl;
extern Shapes shapes;
