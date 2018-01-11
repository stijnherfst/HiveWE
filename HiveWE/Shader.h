#pragma once

class Shader : public Resource {
public:
	GLuint program;

	static constexpr const char* name = "Shader";

	Shader(const std::initializer_list<std::string> paths);

	virtual ~Shader() {

	}

	void use() {
		gl->glUseProgram(program);
	}
};