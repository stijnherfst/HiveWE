#pragma once

#include <QObject>

class GLWidget : public QOpenGLWidget {
	Q_OBJECT

public:
	QElapsedTimer elapsed_timer;
	GLuint vao;

	TPSCamera tps_camera;
	FPSCamera fps_camera;

	explicit GLWidget(QWidget* parent);
	~GLWidget() = default;

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
};