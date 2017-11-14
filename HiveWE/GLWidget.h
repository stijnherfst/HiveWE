#pragma once

#include <QObject>

class GLWidget : public QOpenGLWidget {
	Q_OBJECT

public:
	int previousMouseX = 0;
	int previousMouseY = 0;

	std::unordered_set<int> keysPressed;
	QElapsedTimer elapsedTimer;

	GLWidget(QWidget* parent);
	~GLWidget();

	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void updateScene();
	void paintGL() override;

	void keyPressEvent(QKeyEvent *e);
	void keyReleaseEvent(QKeyEvent *e);
};