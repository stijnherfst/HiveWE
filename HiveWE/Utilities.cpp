#include "stdafx.h"

QOpenGLFunctions_4_5_Core* gl;

unsigned char* SOIL_load_image_flipped(const char *filename, int *width, int *height, int *channels, int force_channels) {
	unsigned char* image = SOIL_load_image(filename, width, height, channels, force_channels);

	int i, j;
	for (j = 0; j * 2 < *height; ++j)
	{
		int index1 = j * *width * *channels;
		int index2 = (*height - 1 - j) * *width * *channels;
		for (i = *width * *channels; i > 0; --i)
		{
			unsigned char temp = image[index1];
			image[index1] = image[index2];
			image[index2] = temp;
			++index1;
			++index2;
		}
	}

	return image;
}

std::vector<std::string> split(const std::string& string, char delimiter) {
	std::vector<std::string> elems;
	std::stringstream ss(string);

	std::string item;
	while (std::getline(ss, item, delimiter)) {
		elems.push_back(item);
	}
	return elems;
}

GLuint compile_shader(const fs::path vertex_shader, const fs::path fragment_shader) {
	return compile_shader(read_text_file(vertex_shader.string()), read_text_file(fragment_shader.string()));
}

GLuint compile_shader(const std::string vertexShader, std::string fragmentShader) {
	char buffer[512];
	GLint status;

	GLuint vertex = gl->glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment = gl->glCreateShader(GL_FRAGMENT_SHADER);

	// Vertex Shader
	const char* source = vertexShader.c_str();
	gl->glShaderSource(vertex, 1, &source, NULL);
	gl->glCompileShader(vertex);


	gl->glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);
	gl->glGetShaderInfoLog(vertex, 512, NULL, buffer);
	std::cout << buffer << std::endl;

	// Fragment Shader
	source = fragmentShader.c_str();
	gl->glShaderSource(fragment, 1, &source, NULL);
	gl->glCompileShader(fragment);

	gl->glGetShaderiv(fragment, GL_COMPILE_STATUS, &status);
	gl->glGetShaderInfoLog(fragment, 512, NULL, buffer);
	std::cout << buffer << std::endl;

	// Link
	GLuint shader = gl->glCreateProgram();
	gl->glAttachShader(shader, vertex);
	gl->glAttachShader(shader, fragment);
	gl->glLinkProgram(shader);

	gl->glGetProgramiv(shader, GL_LINK_STATUS, &status);
	if (!status) {
		gl->glGetProgramInfoLog(shader, 512, NULL, buffer);
		std::cout << buffer << std::endl;
	}

	gl->glDeleteShader(vertex);
	gl->glDeleteShader(fragment);

	return shader;
}

std::string read_text_file(std::string path) {
	std::ifstream textfile(path.c_str());
	std::string line;
	std::string text;

	if (!textfile.is_open())
		return "";

	while (getline(textfile, line)) {
		text += line + "\n";
	}

	return text;
}