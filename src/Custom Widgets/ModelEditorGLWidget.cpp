#include "ModelEditorGLWidget.h"

#include "fmt/format.h"

#include <QTimer>
#include <QOpenGLFunctions_4_5_Core>
#include <QPainter>

#include "Utilities.h"
#include "InputHandler.h"

#include "HiveWE.h"

ModelEditorGLWidget::ModelEditorGLWidget(QWidget* parent) : QOpenGLWidget(parent) {
	//QTimer::singleShot(16, this, &GLWidget::update_scene);

	setMouseTracking(true);
	setFocus();
	setFocusPolicy(Qt::WheelFocus);
}



void ModelEditorGLWidget::initializeGL() {
	gl = new QOpenGLFunctions_4_5_Core;
	gl->initializeOpenGLFunctions();
	
	gl->glEnable(GL_DEPTH_TEST);
	gl->glEnable(GL_CULL_FACE);
	gl->glDepthFunc(GL_LEQUAL);
	gl->glEnable(GL_BLEND);
	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gl->glClearColor(0, 0, 0, 1);

	gl->glGenVertexArrays(1, &vao);
	gl->glBindVertexArray(vao);

	begin = std::chrono::steady_clock::now();
}

void ModelEditorGLWidget::resizeGL(const int w, const int h) {
	gl->glViewport(0, 0, w, h);

	delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	camera->aspect_ratio = double(w) / h;

	camera->update(delta);
}

void ModelEditorGLWidget::update_scene() {
	delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	elapsed_timer.start();

	update();

	QTimer::singleShot(16.67 - std::clamp(delta, 0.001, 16.60), this, &ModelEditorGLWidget::update_scene);
}

void ModelEditorGLWidget::paintGL() {
	gl->glEnable(GL_DEPTH_TEST);
	gl->glDepthMask(true);
	gl->glClearColor(0.f, 0.f, 0.f, 1.f);
	gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	gl->glBindVertexArray(vao);

	// Draw MDX here
}

void ModelEditorGLWidget::keyPressEvent(QKeyEvent* event) {
	input_handler.keys_pressed.emplace(event->key());
}

void ModelEditorGLWidget::keyReleaseEvent(QKeyEvent* event) {
	input_handler.keys_pressed.erase(event->key());
}

void ModelEditorGLWidget::mouseMoveEvent(QMouseEvent* event) {
	input_handler.mouse_move_event(event);
	camera->mouse_move_event(event);
}

void ModelEditorGLWidget::mousePressEvent(QMouseEvent* event) {
	makeCurrent();

	camera->mouse_press_event(event);
}

void ModelEditorGLWidget::mouseReleaseEvent(QMouseEvent* event) {
	camera->mouse_release_event(event);
}

void ModelEditorGLWidget::wheelEvent(QWheelEvent* event) {
	camera->mouse_scroll_event(event);
}