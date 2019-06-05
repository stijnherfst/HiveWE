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
	QIcon trigger_comment_icon;

	QIcon event_icon;
	QIcon condition_icon;
	QIcon action_icon;

	std::unordered_map<std::string, QIcon> trigger_icons;

	std::unordered_map<int, QTreeWidgetItem*> folders;
	std::unordered_map<QTreeWidgetItem*, std::reference_wrapper<Trigger>> files;

	void item_clicked(QTreeWidgetItem* item);
	void show_gui_trigger(QTreeWidget* edit, Trigger& trigger);

	std::string get_parameters_names(const std::vector<std::string>& string_parameters, const std::vector<TriggerParameter>& parameters) const;

signals:
	void save_changes();
};