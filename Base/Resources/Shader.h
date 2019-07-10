#pragma once

#include "utilities.h"

#include "ResourceManager.h"

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