#pragma once

#include <QObject>
#include <QOpenGLWidget>

#include "Camera.h"
#include <QElapsedTimer>
#include "EditableMesh.h"
#include <ModelEditor/ModelEditorCamera.h>

class ModelEditorGLWidget : public QOpenGLWidget {
	Q_OBJECT

public:
	QElapsedTimer elapsed_timer;
	GLuint vao;

	//mdx::MDX model;

	double delta = 0.0;
	std::chrono::steady_clock::time_point begin;


	explicit ModelEditorGLWidget(QWidget* parent);
	~ModelEditorGLWidget() = default;

	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void update_scene();
	void paintGL() override;

	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

	std::shared_ptr<EditableMesh> mesh;
	SkeletalModelInstance skeleton;

	std::shared_ptr<Shader> shader;

	ModelEditorCamera camera;
	InputHandler my_input_handler;
};