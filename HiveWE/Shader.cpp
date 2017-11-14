#include "stdafx.h"

Shader::Shader(const std::initializer_list<std::string> paths) {
	program = compile_shader(fs::path(*paths.begin()), fs::path(*(paths.begin() + 1))); // Not that pretty
}