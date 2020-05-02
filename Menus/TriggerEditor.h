#pragma once

#include "ui_TriggerEditor.h"

#include <QTreeView>
#include <QTreeWidget>

#include <vector>
#include <string>
#include <unordered_map>
#include "Triggers.h"

#include "DockManager.h"
#include "DockAreaWidget.h"
#include "TriggerExplorer.h"

class TriggerEditor : public QMainWindow {
	Q_OBJECT

public:
	TriggerEditor(QWidget* parent = nullptr);
	~TriggerEditor();

	void save_changes();
private:
	Ui::TriggerEditor ui;

	ads::CDockManager* dock_manager = new ads::CDockManager();
	ads::CDockAreaWidget* dock_area = nullptr;

	TriggerExplorer* explorer = new TriggerExplorer;
	TreeModel* model;

	QIcon folder_icon;
	QIcon gui_icon;
	QIcon gui_icon_disabled;
	QIcon script_icon;
	QIcon script_icon_disabled;
	QIcon variable_icon;
	QIcon comment_icon;

	QIcon event_icon;
	QIcon condition_icon;
	QIcon action_icon;

	std::unordered_map<std::string, QIcon> trigger_icons;

	void focus_search_window();

	void item_clicked(const QModelIndex& index);
	void show_gui_trigger(QTreeWidget* edit, const Trigger& trigger);

	std::string get_parameters_names(const std::vector<std::string>& string_parameters, const std::vector<TriggerParameter>& parameters) const;
};