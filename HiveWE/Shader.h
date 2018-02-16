#pragma once

class Shader : public Resource {
public:
	GLuint program;

	static constexpr const char* name = "Shader";

	Shader(const std::initializer_list<fs::path> paths);

	virtual ~Shader() = default;

	void use() {
		gl->glUseProgram(program);
	}
};