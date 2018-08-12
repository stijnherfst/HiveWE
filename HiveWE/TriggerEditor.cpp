#include "stdafx.h"

TriggerEditor::TriggerEditor(QWidget* parent) : QMainWindow(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	ui.splitter->setSizes({ 10000, 20000 });
	show();

	QFileIconProvider icons;
	folder_icon = icons.icon(QFileIconProvider::Folder);
	file_icon = icons.icon(QFileIconProvider::File);

	trigger_comment_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerComment") + ".blp");

	event_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_Event"));
	condition_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_Condition"));
	action_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_Action"));

	for (auto&& i : map.triggers.categories) {
		QTreeWidgetItem* item = new QTreeWidgetItem(ui.explorer);
		item->setData(0, Qt::EditRole, QString::fromStdString(i.name));
		item->setIcon(0, folder_icon);
		folders[i.id] = item;
	}

	for (auto&& i : map.triggers.triggers) {
		QTreeWidgetItem* item = new QTreeWidgetItem(folders[i.category_id]);
		item->setData(0, Qt::EditRole, QString::fromStdString(i.name));
		if (i.is_comment) {
			item->setIcon(0, trigger_comment_icon);
		} else {
			item->setIcon(0, file_icon);
		}

		files.emplace(item, i);
	}

	connect(ui.explorer, &QTreeWidget::itemDoubleClicked, this, &TriggerEditor::item_clicked);
	connect(ui.editor, &QTabWidget::tabCloseRequested, [&](int index) { delete ui.editor->widget(index); });
}

void TriggerEditor::item_clicked(QTreeWidgetItem* item) {
	if (files.find(item) == files.end()) {
		return;
	}
	Trigger& trigger = files.at(item).get();

	if (trigger.is_comment) {
		return;
	}

	// Check if trigger is already open and if so focus it
	for (int i = 0; i < ui.editor->count(); i++) {
		if (ui.editor->widget(i)->property("TriggerID").toInt() == trigger.id) {
			ui.editor->setCurrentIndex(i);
			return;
		}
	}

	QWidget* tab = new QWidget;
	tab->setProperty("TriggerID", trigger.id);

	QVBoxLayout* layout = new QVBoxLayout(tab);

	if (!trigger.custom_text.empty()) {
		JassEditor* edit = new JassEditor;
		layout->addWidget(edit);
		edit->setText(QString::fromStdString(trigger.custom_text));
	} else {
		QTreeWidget* edit = new QTreeWidget;
		edit->setHeaderHidden(true);
		edit->setUniformRowHeights(true);
		layout->addWidget(edit);

		QTreeWidgetItem* events = new QTreeWidgetItem(edit);
		events->setText(0, "Events");
		events->setIcon(0, event_icon);

		QTreeWidgetItem* conditions = new QTreeWidgetItem(edit);
		conditions->setText(0, "Conditions");
		conditions->setIcon(0, condition_icon);

		QTreeWidgetItem* actions = new QTreeWidgetItem(edit);
		actions->setText(0, "Actions");
		actions->setIcon(0, action_icon);

		std::function<void(QTreeWidgetItem*, ECA&)> recurse = [&](QTreeWidgetItem* parent, ECA& i) {
			QTreeWidgetItem* eca = new QTreeWidgetItem(parent);
			std::string category;

			switch (i.type) {
				case ECA::Type::event:
					eca->setText(0, QString::fromStdString(map.triggers.trigger_strings.data("TriggerEventStrings", i.name)));
					category = map.triggers.trigger_data.data("TriggerEvents", "_" + i.name + "_Category");
					break;
				case ECA::Type::condition:
					eca->setText(0, QString::fromStdString(map.triggers.trigger_strings.data("TriggerConditionStrings", i.name)));
					category = map.triggers.trigger_data.data("TriggerConditions", "_" + i.name + "_Category");
					break;
				case ECA::Type::action:
					eca->setText(0, QString::fromStdString(map.triggers.trigger_strings.data("TriggerActionStrings", i.name)));
					category = map.triggers.trigger_data.data("TriggerActions", "_" + i.name + "_Category");
					break;
			}

			if (auto found = trigger_icons.find(category); found == trigger_icons.end()) {
				std::string icon_path = map.triggers.trigger_data.data("TriggerCategories", category, 1);
				std::string final_path = icon_path + ".blp";
				QIcon icon = texture_to_icon(final_path);
				trigger_icons[category] = icon;
				eca->setIcon(0, icon);
			} else {
				eca->setIcon(0, found->second);
			}
			for (auto&& j : i.ecas) {
				recurse(eca, j);
			}
		};


		for (auto&& i : trigger.lines) {
			recurse(nullptr, i);

			QTreeWidgetItem* eca = new QTreeWidgetItem;
			std::string category;
			std::vector<std::string> string_parameters;

			switch (i.type) {
				case ECA::Type::event:
					events->addChild(eca);
					string_parameters = map.triggers.trigger_strings.whole_data("TriggerEventStrings", i.name);
					category = map.triggers.trigger_data.data("TriggerEvents", "_" + i.name + "_Category");
					break;
				case ECA::Type::condition:
					conditions->addChild(eca);
					string_parameters = map.triggers.trigger_strings.whole_data("TriggerConditionStrings", i.name);
					category = map.triggers.trigger_data.data("TriggerConditions", "_" + i.name + "_Category");
					break;
				case ECA::Type::action:
					actions->addChild(eca);
					string_parameters = map.triggers.trigger_strings.whole_data("TriggerActionStrings", i.name);
					category = map.triggers.trigger_data.data("TriggerActions", "_" + i.name + "_Category");
					break;
			}

			eca->setText(0, QString::fromStdString(get_parameters_names(string_parameters, i.parameters)));

			if (auto found = trigger_icons.find(category); found == trigger_icons.end()) {
				std::string icon_path = map.triggers.trigger_data.data("TriggerCategories", category, 1);
				std::string final_path = icon_path + ".blp";
				QIcon icon = texture_to_icon(final_path);
				trigger_icons[category] = icon;
				eca->setIcon(0, icon);
			} else {
				eca->setIcon(0, found->second);
			}
			for (auto&& j : i.ecas) {
				recurse(eca, j);
			}
		}

		edit->expandAll();
	}

	ui.editor->addTab(tab, QString::fromStdString(trigger.name));
	ui.editor->setCurrentWidget(tab);
}

std::string TriggerEditor::get_parameters_names(std::vector<std::string> string_parameters, std::vector<TriggerParameter>& parameters) {
	std::string result = "";

	int current_parameter = 0;
	for (auto&& i : string_parameters) {
		if (i.size() && i.front() == '~') {
			TriggerParameter& j = parameters[current_parameter];

			std::vector<std::string> sub_string_parameters;
			if (j.has_sub_parameter) {
				switch (j.sub_parameter.type) {
				case TriggerSubParameter::Type::events:
					sub_string_parameters = map.triggers.trigger_strings.whole_data("TriggerEventStrings", j.sub_parameter.name);
					break;
				case TriggerSubParameter::Type::conditions:
					sub_string_parameters = map.triggers.trigger_strings.whole_data("TriggerConditionStrings", j.sub_parameter.name);
					break;
				case TriggerSubParameter::Type::actions:
					sub_string_parameters = map.triggers.trigger_strings.whole_data("TriggerActionStrings", j.sub_parameter.name);
					break;
				case TriggerSubParameter::Type::calls:
					sub_string_parameters = map.triggers.trigger_strings.whole_data("TriggerCallStrings", j.sub_parameter.name);
					break;
				}
				result += "(" + get_parameters_names(sub_string_parameters, j.sub_parameter.parameters) + ")";
			} else {
				switch (j.type) {
					case TriggerParameter::Type::preset:
						result += map.triggers.trigger_data.data("TriggerParams", j.value, 3);
						break;
					case TriggerParameter::Type::string:
						if (j.value.size() == 4) {
							// Might be a unit
							if (map.units.units_slk.header_to_row.find(j.value) != map.units.units_slk.header_to_row.end()) {
								result += map.units.units_slk.data("Name", j.value);
							} else {
								result += j.value;
							}
						} else if (j.value.size() > 8 && j.value.substr(0, 7) == "TRIGSTR") {
							result += map.trigger_strings.strings[j.value];
						} else {
							result += j.value;

						}
						break;
					case TriggerParameter::Type::variable: {
						if (j.value.size() > 7 && j.value.substr(0, 7) == "gg_unit") {
							std::string type = j.value.substr(8, 4);
							std::string instance = j.value.substr(13);
							result += map.units.units_slk.data("Name", type);
							result += " " + instance;
						} else {
							std::string type = map.triggers.variables[j.value].type;
							if (type == "unit") {
								std::cout << "test\n";
							} else {
								result += j.value;
							}
						}
						break;
					}
					default:
						result += j.value;
				}
			}
			current_parameter++;
		} else {
			result += i;
		}
	}

	return result;
}