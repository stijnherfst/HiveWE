#pragma once

#include <glad/glad.h>
#include <memory>
#define QT_NO_OPENGL
#include <QObject>
#include <QOpenGLWidget>
#include "qt_imgui/qt_imGui.h"
#include <model_editor/model_editor_camera.h>

import EditableMesh;
import SkeletalModelInstance;
import MDX;
import Shader;

class ModelEditorGLWidget: public QOpenGLWidget {
	Q_OBJECT

  public:
	QElapsedTimer elapsed_timer;
	GLuint vao;

	double delta = 0.0;

	ModelEditorGLWidget() = delete;
	explicit ModelEditorGLWidget(QWidget* parent, std::shared_ptr<mdx::MDX> model);
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

	std::shared_ptr<mdx::MDX> mdx;
	std::shared_ptr<EditableMesh> mesh;
	SkeletalModelInstance skeleton;
	std::shared_ptr<Shader> shader;

	int64_t optimization_file_size_reduction = 0;
	float optimization_file_size_reduction_percent = 0.f;

	QtImGui::RenderRef ref = nullptr;
	ModelEditorCamera camera;
};
