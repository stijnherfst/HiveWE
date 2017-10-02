#include "stdafx.h"

float vertices[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	0.0f,  1.0f, 0.0f,
};

const char* vertexSource =  R"glsl(
	#version 330 

	layout (location = 0) in vec3 vPosition;

	uniform mat4 MVP;

	void main() { 
		 gl_Position = MVP * vec4(vPosition, 1);
	}
)glsl";

const char* fragmentSource = R"glsl(
	#version 330

	out vec4 outColor;

	void main() {
		outColor = vec4(1.0, 1.0, 1.0, 1.0);
	}
)glsl";

cameraStruct camera;

GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent) {

}

GLWidget::~GLWidget() {
}

GLuint vbo;
GLuint shader;

void GLWidget::initializeGL() {
	initializeOpenGLFunctions();
	glClearColor(0, 0, 0, 1);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	shader = compileShader(vertexSource, fragmentSource);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

}

void GLWidget::resizeGL(int w, int h) {
	glViewport(0, 0, w, h);
}


void GLWidget::paintGL() {
	glClearColor(0, 0, 0, 1);

	glUseProgram(shader);

	glm::mat4 Model = glm::mat4(1.0f);
	Model = glm::translate(Model, glm::vec3(0, 0, 0));
	glm::mat4 MVP = camera.projection * camera.view * Model;
	glUniformMatrix4fv(glGetUniformLocation(shader, "MVP"), 1, GL_FALSE, &MVP[0][0]);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}

GLuint GLWidget::compileShader(const char* vertexShader, const char* fragmentShader) {
	char buffer[512];
	GLint status;

	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

	// Vertex Shader
	glShaderSource(vertex, 1, &vertexShader, NULL);
	glCompileShader(vertex);

	
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);
	glGetShaderInfoLog(vertex, 512, NULL, buffer);
	std::cout << buffer << std::endl;

	// Fragment Shader
	glShaderSource(fragment, 1, &fragmentShader, NULL);
	glCompileShader(fragment);

	glGetShaderiv(fragment, GL_COMPILE_STATUS, &status);
	glGetShaderInfoLog(fragment, 512, NULL, buffer);
	std::cout << buffer << std::endl;

	// Link
	GLuint shader = glCreateProgram();
	glAttachShader(shader, vertex);
	glAttachShader(shader, fragment);

	glLinkProgram(shader);

	return shader;
}