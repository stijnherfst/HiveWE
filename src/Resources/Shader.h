#pragma once

#include "OpenGLUtilities.h"

import ResourceManager;

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