#pragma once

#include "ui_TriggerEditor.h"

class TriggerEditor : public QMainWindow {
	Q_OBJECT

public:
	TriggerEditor(QWidget* parent = nullptr);

private:
	Ui::TriggerEditor ui;

	QIcon folder_icon;
	QIcon file_icon;

	std::unordered_map<int, QTreeWidgetItem*> folders;
	std::unordered_map<QTreeWidgetItem*, std::reference_wrapper<Trigger>> files;

	void item_clicked(QTreeWidgetItem* item);
};