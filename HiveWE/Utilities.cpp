#include "stdafx.h"

QOpenGLExtraFunctions* gl;
Shapes shapes;

//unsigned char* SOIL_load_image_flipped(const char *filename, int *width, int *height, int *channels, int force_channels) {
//	unsigned char* image = SOIL_load_image(filename, width, height, channels, force_channels);
//
//	int i, j;
//	for (j = 0; j * 2 < *height; ++j)
//	{
//		int index1 = j * *width * *channels;
//		int index2 = (*height - 1 - j) * *width * *channels;
//		for (i = *width * *channels; i > 0; --i)
//		{
//			unsigned char temp = image[index1];
//			image[index1] = image[index2];
//			image[index2] = temp;
//			++index1;
//			++index2;
//		}
//	}
//
//	return image;
//}

void Shapes::init() {
	gl->glGenBuffers( 1, &vertex_buffer );
	gl->glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer );
	gl->glBufferData( GL_ARRAY_BUFFER, quad_vertices.size( ) * sizeof( glm::vec2 ), quad_vertices.data( ), GL_STATIC_DRAW );

	gl->glGenBuffers( 1, &index_buffer );
	gl->glBindBuffer( GL_ARRAY_BUFFER, index_buffer );
	gl->glBufferData( GL_ARRAY_BUFFER, quad_indices.size( ) * sizeof( unsigned int ) * 3, quad_indices.data( ), GL_STATIC_DRAW );
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

GLuint compile_shader(const fs::path& vertex_shader, const fs::path& fragment_shader) {
	char buffer[512];
	GLint status;

	std::string vertex_source = read_text_file(vertex_shader.string());
	std::string fragment_source = read_text_file(fragment_shader.string());

	const GLuint vertex = gl->glCreateShader(GL_VERTEX_SHADER);
	const GLuint fragment = gl->glCreateShader(GL_FRAGMENT_SHADER);

	// Vertex Shader
	const char* source = vertex_source.c_str();
	gl->glShaderSource(vertex, 1, &source, nullptr);
	gl->glCompileShader(vertex);


	gl->glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);
	gl->glGetShaderInfoLog(vertex, 512, nullptr, buffer);
	std::cout << buffer << std::endl;

	// Fragment Shader
	source = fragment_source.c_str();
	gl->glShaderSource(fragment, 1, &source, nullptr);
	gl->glCompileShader(fragment);

	gl->glGetShaderiv(fragment, GL_COMPILE_STATUS, &status);
	gl->glGetShaderInfoLog(fragment, 512, nullptr, buffer);
	std::cout << buffer << std::endl;

	// Link
	const GLuint shader = gl->glCreateProgram();
	gl->glAttachShader(shader, vertex);
	gl->glAttachShader(shader, fragment);
	gl->glLinkProgram(shader);

	gl->glGetProgramiv(shader, GL_LINK_STATUS, &status);
	if (!status) {
		gl->glGetProgramInfoLog(shader, 512, nullptr, buffer);
		std::cout << buffer << std::endl;
	}

	gl->glDeleteShader(vertex);
	gl->glDeleteShader(fragment);

	return shader;
}

std::string read_text_file(const std::string& path) {
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

fs::path find_warcraft_directory() {
	if (fs::exists(L"C:/Program Files (x86)/Warcraft III/War3Patch.mpq")) {
		return L"C:/Program Files (x86)/Warcraft III/";
	} else if (fs::exists(L"D:/Program Files (x86)/Warcraft III/War3Patch.mpq")) {
		return L"D:/Program Files (x86)/Warcraft III/";
	} else {
		return L"";
	}
}

QIcon texture_to_icon(uint8_t* data, int width, int height) {
	QImage temp_image = QImage(data, width, height, QImage::Format::Format_ARGB32);
	const int size = height / 4;

	auto pix = QPixmap::fromImage(temp_image.copy(0, 0, size, size));

	QIcon icon;
	icon.addPixmap(pix, QIcon::Normal, QIcon::Off);

	QPainter painter(&pix);
	painter.fillRect(0, 0, size, size, QColor(255, 255, 0, 64));
	painter.end();

	icon.addPixmap(pix, QIcon::Normal, QIcon::On);

	return icon;
};