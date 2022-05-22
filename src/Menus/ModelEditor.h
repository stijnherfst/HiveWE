#pragma once

#include <QMainWindow>

#include "ui_ModelEditor.h"

#include "DockManager.h"
#include "DockAreaWidget.h"
#include <QTreeView>
#include <QOpenGLWidget>

class ModelEditor : public QMainWindow {
	Q_OBJECT

public:
	ModelEditor(QWidget* parent = nullptr);

private:
	Ui::ModelEditor ui;

	//QOpenGLWidget opengl_widget;

	//std::shared_ptr<QIconResource> custom_unit_icon;
};