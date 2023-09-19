module;

#include <filesystem>
#include <glad/glad.h>
#include <print>

export module Shader;

import ResourceManager;
import Utilities;

namespace fs = std::filesystem;

export class Shader : public Resource {
  public:
	GLuint program;

	static constexpr const char* name = "Shader";

	// vertex: .vert
	// fragment: .frag
	// compute: .comp
	explicit Shader(std::initializer_list<fs::path> paths) {
		program = glCreateProgram();

		for (const auto& path : paths) {
			GLuint shader;
			if (path.extension() == ".vert") {
				shader = glCreateShader(GL_VERTEX_SHADER);
			} else if (path.extension() == ".frag") {
				shader = glCreateShader(GL_FRAGMENT_SHADER);
			} else if (path.extension() == ".comp") {
				shader = glCreateShader(GL_COMPUTE_SHADER);
			}

			static char buffer[512];
			GLint status;
			const std::string source = read_text_file(path);
			const char* source_c_str = source.c_str();
			glShaderSource(shader, 1, &source_c_str, nullptr);
			glCompileShader(shader);

			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (!status) {
				glGetShaderInfoLog(shader, 512, nullptr, buffer);
				std::print("{}\n{}\n", path.string(), buffer);
			}

			glAttachShader(program, shader);
		}

		glLinkProgram(program);
		static char buffer[512];
		GLint status;
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if (!status) {
			glGetProgramInfoLog(program, 512, nullptr, buffer);

			std::print("Failed to link\n");
			for (const auto& path : paths) {
				std::print("{}\n", path.string());
			}
			std::print("{}\n", buffer);
		}
	}

	~Shader() {
		glDeleteProgram(program);
	}

	void use() const {
		glUseProgram(program);
	}
};