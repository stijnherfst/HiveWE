#pragma once

class Shader : public Resource {
public:
	GLuint program;

	Shader(const std::initializer_list<std::string> paths);

	virtual ~Shader() {

	}

	void use() {
		gl->glUseProgram(program);
	}
};