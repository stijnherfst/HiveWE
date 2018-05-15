#pragma once

#include <QObject>

class GLWidget : public QOpenGLWidget {
	Q_OBJECT

public:
	QTimer timer;
	QElapsedTimer elapsed_timer;
	GLuint vao;

	explicit GLWidget(QWidget* parent);
	~GLWidget() = default;

	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void update_scene();
	void paintGL() override;

	void keyPressEvent(QKeyEvent* e) override;
	void keyReleaseEvent(QKeyEvent* e) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent* event) override;
};