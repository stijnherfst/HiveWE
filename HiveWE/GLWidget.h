#pragma once

#include <QObject>

class GLWidget : public QOpenGLWidget {
	Q_OBJECT

public:
	GLWidget(QWidget* parent);
	~GLWidget();

	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

	GLuint compileShader(const char* vertexShader, const char* fragmentShader);
};

extern QOpenGLFunctions_4_2_Core* gl;