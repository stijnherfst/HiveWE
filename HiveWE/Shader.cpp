#include "stdafx.h"

Shader::Shader(const std::initializer_list<fs::path> paths) {
	program = compile_shader(*paths.begin(), *(paths.begin() + 1));
}