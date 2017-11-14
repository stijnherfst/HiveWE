#pragma once

extern QOpenGLFunctions_4_2_Core* gl;

unsigned char* SOIL_load_image_flipped(const char *filename, int *width, int *height, int *channels, int force_channels);

std::vector<std::string> split(const std::string& string, char delimiter);

GLuint compile_shader(const fs::path vertex_shader, const fs::path fragment_shader);
GLuint compile_shader(const std::string vertexShader, const std::string fragmentShader);

std::string read_text_file(std::string path);