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

	layout (binding = 0) uniform sampler2DArray textureArray;

	in vec3 UVW;

	out vec4 outColor;

	void main() {
		outColor = texture(textureArray, UVW);
	}
)glsl";

cameraStruct camera;

GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent) {
	QTimer *timer = new QTimer;
	connect(timer, &QTimer::timeout, this, &GLWidget::updateScene);
	timer->start(30);

	grabMouse();
	setMouseTracking(true);
	setFocus();
}

GLWidget::~GLWidget() {
}

Map map;
GLuint shader;

void GLWidget::initializeGL() {
	gl = new QOpenGLFunctions_4_2_Core;
	gl->initializeOpenGLFunctions();

	gl->glEnable(GL_DEPTH_TEST);
	gl->glDepthFunc(GL_LEQUAL);
	gl->glEnable(GL_BLEND);
	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gl->glClearColor(0, 0, 0, 1);
	//gl->glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	GLuint vao;
	gl->glGenVertexArrays(1, &vao);
	gl->glBindVertexArray(vao);

	shader = compileShader(vertexSource, fragmentSource);

	map.load(L"Data/t.w3x");
}

void GLWidget::resizeGL(int w, int h) {
	gl->glViewport(0, 0, w, h);
	camera.aspectRatio = (float)w / h;
	camera.update();
}

void GLWidget::updateScene() {
	double delta = elapsedTimer.nsecsElapsed() / 1'000'000'000.0;
	elapsedTimer.start();

	float speed = 5;
	if (keysPressed.count(Qt::Key::Key_W)) {
		camera.position += camera.direction * speed * (float)delta;
	} else if (keysPressed.count(Qt::Key::Key_S)) {
		camera.position -= camera.direction * speed * (float)delta;
	}

	if (keysPressed.count(Qt::Key::Key_A)) {
		camera.position -= glm::normalize(glm::cross(camera.direction, camera.up)) * speed * (float)delta;
	} else if (keysPressed.count(Qt::Key::Key_D)) {
		camera.position += glm::normalize(glm::cross(camera.direction, camera.up)) * speed * (float)delta;
	}

	if (keysPressed.count(Qt::Key::Key_Space)) {
		camera.position.z += 1 * speed * (float)delta;
	} else if (keysPressed.count(Qt::Key::Key_Control)) {
		camera.position.z -= 1 * speed * (float)delta;
	}

	
	int diffx = mapFromGlobal(QCursor::pos()).x() - previousMouseX;
	int diffy = previousMouseY - mapFromGlobal(QCursor::pos()).y();
	previousMouseX = mapFromGlobal(QCursor::pos()).x();
	previousMouseY = mapFromGlobal(QCursor::pos()).y();
	camera.horizontalAngle += diffx * 0.1 * speed * (float)delta;
	camera.verticalAngle += diffy * 0.1 * speed * (float)delta;
	camera.update();

	update();
}

void GLWidget::paintGL() {
	gl->glClearColor(0, 0, 0, 1);

	gl->glUseProgram(shader);
	
	glm::mat4 Model = glm::mat4(1.0f);
	Model = glm::translate(Model, glm::vec3(0, 0, 0));
	glm::mat4 MVP = camera.projection * camera.view * Model;
	gl->glUniformMatrix4fv(gl->glGetUniformLocation(shader, "MVP"), 1, GL_FALSE, &MVP[0][0]);

	map.terrain.render();
}

void GLWidget::keyPressEvent(QKeyEvent *e) {
	keysPressed.emplace(e->key());
	switch (e->key()) {
		case Qt::Key::Key_Escape:
			exit(0);
			break;
	}
}

void GLWidget::keyReleaseEvent(QKeyEvent *e) {
	keysPressed.erase(e->key());
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