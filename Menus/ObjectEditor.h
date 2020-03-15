#pragma once

#include <QMainWindow>

#include "ui_ObjectEditor.h"
#include "UnitTreeModel.h"

#include "UnitModel.h"
#include "DockManager.h"
#include "DockAreaWidget.h"

class ObjectEditor : public QMainWindow {
	Q_OBJECT

public:
	ObjectEditor(QWidget* parent = nullptr);

private:
	Ui::ObjectEditor ui;

	ads::CDockManager* dock_manager = new ads::CDockManager();
	ads::CDockAreaWidget* dock_area = nullptr;

	QTreeView* explorer = new QTreeView;
	UnitTreeModel* explorerModel;

	UnitModel* model;

	void item_clicked(const QModelIndex& index);
};