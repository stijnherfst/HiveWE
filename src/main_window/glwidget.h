#pragma once

#define QT_NO_OPENGL
#include <glad/glad.h>

#include <QObject>
#include <QOpenGLWidget>
#include <QElapsedTimer>

class GLWidget : public QOpenGLWidget {
	Q_OBJECT

public:
	QElapsedTimer elapsed_timer;
	GLuint vao;

	double delta = 0.0;

	explicit GLWidget(QWidget* parent);
	~GLWidget() = default;

	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
};