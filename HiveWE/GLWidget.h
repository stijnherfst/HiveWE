#pragma once

#include <QObject>

class GLWidget : public QOpenGLWidget {
	Q_OBJECT

public:
	QElapsedTimer elapsedTimer;
	GLuint vao;

	GLWidget(QWidget* parent);
	~GLWidget();

	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void updateScene();
	void paintGL() override;

	void keyPressEvent(QKeyEvent* e) override;
	void keyReleaseEvent(QKeyEvent* e) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
};