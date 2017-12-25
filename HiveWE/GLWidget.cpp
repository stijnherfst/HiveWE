#include "stdafx.h"

cameraStruct camera;

GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent) {
	QTimer *timer = new QTimer;
	connect(timer, &QTimer::timeout, this, &GLWidget::updateScene);
	timer->start(16);

	grabMouse();
	setMouseTracking(true);
	setFocus();
}

GLWidget::~GLWidget() {
}

Map map;
std::shared_ptr<StaticMesh> mesh;

void GLWidget::initializeGL() {
	gl = new QOpenGLFunctions_4_5_Core;
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

	map.load(L"Data/sa.w3x");

	mesh = resource_manager.load<StaticMesh>("Units\\Human\\Footman\\Footman.mdx");
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
	if (keysPressed.count(Qt::Key::Key_Shift)) {
		speed = 20;
	}

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

	map.terrain.current_texture += std::max(0.f, map.terrain.animation_rate * (float)delta);
	if (map.terrain.current_texture >= map.terrain.water_textures_nr) {
		map.terrain.current_texture = 0;
	}

	update();
}

void GLWidget::paintGL() {

	gl->glClearColor(0, 0, 0, 1);
	
	map.terrain.render();
	map.pathing_map.render();
	map.doodads.render();


	//gl->glUseProgram(shader2);

	//glm::mat4 Model = glm::mat4(1.0f);
	//Model = glm::translate(Model, glm::vec3(0, 0, 0));
	//MVP = camera.projection * camera.view * Model;
	//gl->glUniformMatrix4fv(2, "MVP"), 1, GL_FALSE, &MVP[0][0]);
	//mesh.get()->render();

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