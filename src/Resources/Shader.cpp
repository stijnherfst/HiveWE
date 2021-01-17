#include "Shader.h"

Shader::Shader(std::initializer_list<fs::path> paths) {
	program = compile_shader(*paths.begin(), *(paths.begin() + 1));
}