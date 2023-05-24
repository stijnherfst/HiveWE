#pragma once

#include <QMainWindow>

#include "ui_model_editor.h"

class ModelEditor : public QMainWindow {
	Q_OBJECT

public:
	ModelEditor(QWidget* parent = nullptr);

private:
	Ui::ModelEditor ui;

	//QOpenGLWidget opengl_widget;

	//std::shared_ptr<QIconResource> custom_unit_icon;
};