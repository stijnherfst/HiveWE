#pragma once

#include <glad/glad.h>
#define QT_NO_OPENGL

#include <memory>

#include <QObject>
#include <QOpenGLWidget>

import EditableMesh;
import SkeletalModelInstance;
import MDX;
import Shader;

#include <model_editor/model_editor_camera.h>

class ModelEditorGLWidget : public QOpenGLWidget {
	Q_OBJECT

public:
	QElapsedTimer elapsed_timer;
	GLuint vao;

	double delta = 0.0;

	explicit ModelEditorGLWidget(QWidget* parent);
	~ModelEditorGLWidget() = default;

	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

	std::shared_ptr<EditableMesh> mesh;
	SkeletalModelInstance skeleton;

	int64_t optimization_file_size_reduction = 0;
	float optimization_file_size_reduction_percent = 0.f;
	mdx::MDX::OptimizationStats stats;

	std::shared_ptr<Shader> shader;

	ModelEditorCamera camera;
	InputHandler my_input_handler;
};