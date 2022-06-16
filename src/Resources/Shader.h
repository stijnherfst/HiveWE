#pragma once

#include "Utilities.h"

import ResourceManager;

#include <filesystem>
namespace fs = std::filesystem;

class Shader : public Resource {
public:
	GLuint program;

	static constexpr const char* name = "Shader";

	Shader(std::initializer_list<fs::path> paths);

	virtual ~Shader() = default;

	void use() const {
		gl->glUseProgram(program);
	}
};