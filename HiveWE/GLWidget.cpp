#include "stdafx.h"

QOpenGLFunctions_4_2_Core* gl;

const char* vertexSource =  R"glsl(
	#version 420 core

	layout (location = 0) in vec3 vPosition;
	layout (location = 1) in vec3 vUVW;

	uniform mat4 MVP;

	out vec3 UVW;

	void main() { 
		gl_Position = MVP * vec4(vPosition, 1);
		UVW = vUVW;
	}
)glsl";

const char* fragmentSource = R"glsl(
	#version 420 core

	layout (binding=0) uniform sampler2DArray textureArray;

	in vec3 UVW;

	out vec4 outColor;

	void main() {
		outColor = texture(textureArray, UVW);
	}
)glsl";

cameraStruct camera;

GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent) {

}

GLWidget::~GLWidget() {
}

Terrain terrain;
GLuint shader;

void GLWidget::initializeGL() {
	gl = new QOpenGLFunctions_4_2_Core;
	gl->initializeOpenGLFunctions();

	gl->glClearColor(0, 0, 0, 1);

	GLuint vao;
	gl->glGenVertexArrays(1, &vao);
	gl->glBindVertexArray(vao);

	shader = compileShader(vertexSource, fragmentSource);

	terrain.create();
}

void GLWidget::resizeGL(int w, int h) {
	gl->glViewport(0, 0, w, h);
}


void GLWidget::paintGL() {
	gl->glClearColor(0, 0, 0, 1);

	gl->glUseProgram(shader);
	
	glm::mat4 Model = glm::mat4(1.0f);
	Model = glm::translate(Model, glm::vec3(0, 0, 0));
	glm::mat4 MVP = camera.projection * camera.view * Model;
	gl->glUniformMatrix4fv(gl->glGetUniformLocation(shader, "MVP"), 1, GL_FALSE, &MVP[0][0]);

	terrain.render();
}

GLuint GLWidget::compileShader(const char* vertexShader, const char* fragmentShader) {
	char buffer[512];
	GLint status;

	GLuint vertex = gl->glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment = gl->glCreateShader(GL_FRAGMENT_SHADER);

	// Vertex Shader
	gl->glShaderSource(vertex, 1, &vertexShader, NULL);
	gl->glCompileShader(vertex);

	
	gl->glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);
	gl->glGetShaderInfoLog(vertex, 512, NULL, buffer);
	std::cout << buffer << std::endl;

	// Fragment Shader
	gl->glShaderSource(fragment, 1, &fragmentShader, NULL);
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