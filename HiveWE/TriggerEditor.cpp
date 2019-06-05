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

	for (const auto& i : map->triggers.categories) {
		QTreeWidgetItem* item = new QTreeWidgetItem(ui.explorer);
		item->setData(0, Qt::EditRole, QString::fromStdString(i.name));
		item->setIcon(0, folder_icon);
		folders[i.id] = item;
	}

	for (auto&& i : map->triggers.triggers) {
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
	connect(ui.actionGenerateScript, &QAction::triggered, [&]() {
		save_changes();
		map->triggers.generate_map_script();
		});
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
		connect(this, &TriggerEditor::save_changes, [=]() {
			files.at(item).get().custom_text = edit->text().replace("\r", "").toStdString();
			});
	} else {
		QTreeWidget* edit = new QTreeWidget;
		edit->setHeaderHidden(true);
		edit->setUniformRowHeights(true);
		layout->addWidget(edit);
		show_gui_trigger(edit, trigger);
		edit->expandAll();
	}

	ui.editor->addTab(tab, QString::fromStdString(trigger.name));
	ui.editor->setCurrentWidget(tab);
}

void TriggerEditor::show_gui_trigger(QTreeWidget* edit, Trigger& trigger) {
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

		std::vector<std::string> string_parameters;

		switch (i.type) {
			case ECA::Type::event:
				string_parameters = map->triggers.trigger_strings.whole_data("TriggerEventStrings", i.name);
				category = map->triggers.trigger_data.data("TriggerEvents", "_" + i.name + "_Category");
				break;
			case ECA::Type::condition:
				string_parameters = map->triggers.trigger_strings.whole_data("TriggerConditionStrings", i.name);
				category = map->triggers.trigger_data.data("TriggerConditions", "_" + i.name + "_Category");
				break;
			case ECA::Type::action:
				string_parameters = map->triggers.trigger_strings.whole_data("TriggerActionStrings", i.name);
				category = map->triggers.trigger_data.data("TriggerActions", "_" + i.name + "_Category");
				break;
		}

		eca->setText(0, QString::fromStdString(get_parameters_names(string_parameters, i.parameters)));

		if (auto found = trigger_icons.find(category); found == trigger_icons.end()) {
			std::string icon_path = map->triggers.trigger_data.data("TriggerCategories", category, 1);
			std::string final_path = icon_path + ".blp";
			QIcon icon = texture_to_icon(final_path);
			trigger_icons[category] = icon;
			eca->setIcon(0, icon);
		} else {
			eca->setIcon(0, found->second);
		}

		QTreeWidgetItem* sub1 = eca;
		QTreeWidgetItem* sub2 = eca;
		QTreeWidgetItem* sub3 = eca;

		if (i.name == "AndMultiple" || i.name == "OrMultiple") {
			sub1 = new QTreeWidgetItem(eca, { "Conditions" });
			sub1->setIcon(0, condition_icon);
		} else if (i.name == "IfThenElseMultiple") {
			sub1 = new QTreeWidgetItem(eca, { "If - Conditions" });
			sub1->setIcon(0, condition_icon);
			sub2 = new QTreeWidgetItem(eca, { "Then - Actions" });
			sub2->setIcon(0, action_icon);
			sub3 = new QTreeWidgetItem(eca, { "Else - Actions" });
			sub3->setIcon(0, action_icon);
		} else if (i.name == "ForLoopAMultiple"
			|| i.name == "ForLoopBMultiple"
			|| i.name == "ForLoopVarMultiple"
			|| i.name == "ForLoopA"
			|| i.name == "ForLoopB"
			|| i.name == "ForLoopVar"
			|| i.name == "ForForceMultiple"
			|| i.name == "ForGroupMultiple") {
			sub1 = new QTreeWidgetItem(eca, { "Loop - Actions" });
			sub1->setIcon(0, action_icon);
		}

		for (auto&& j : i.ecas) {
			if (j.group == 0) {
				recurse(sub1, j);
			} else if (j.group == 1) {
				recurse(sub2, j);
			} else if (j.group == 2) {
				recurse(sub3, j);
			}
		}
	};

	for (auto&& i : trigger.lines) {
		if (i.type == ECA::Type::event) {
			recurse(events, i);
		} else if (i.type == ECA::Type::condition) {
			recurse(conditions, i);
		} else if (i.type == ECA::Type::action) {
			recurse(actions, i);
		}
	}
}

std::string TriggerEditor::get_parameters_names(const std::vector<std::string>& string_parameters, const std::vector<TriggerParameter>& parameters) const {
	std::string result;

	int current_parameter = 0;
	for (auto&& i : string_parameters) {
		if (i.empty() || i.front() != '~') {
			result += i;
			continue;
		}
		const TriggerParameter& j = parameters.at(current_parameter);

		std::vector<std::string> sub_string_parameters;
		if (j.has_sub_parameter) {
			switch (j.sub_parameter.type) {
				case TriggerSubParameter::Type::events:
					sub_string_parameters = map->triggers.trigger_strings.whole_data("TriggerEventStrings", j.sub_parameter.name);
					break;
				case TriggerSubParameter::Type::conditions:
					sub_string_parameters = map->triggers.trigger_strings.whole_data("TriggerConditionStrings", j.sub_parameter.name);
					break;
				case TriggerSubParameter::Type::actions:
					sub_string_parameters = map->triggers.trigger_strings.whole_data("TriggerActionStrings", j.sub_parameter.name);
					break;
				case TriggerSubParameter::Type::calls:
					sub_string_parameters = map->triggers.trigger_strings.whole_data("TriggerCallStrings", j.sub_parameter.name);
					break;
			}
			result += "(" + get_parameters_names(sub_string_parameters, j.sub_parameter.parameters) + ")";
		} else {
			switch (j.type) {
				case TriggerParameter::Type::preset:
					result += map->triggers.trigger_data.data("TriggerParams", j.value, 3);
					break;
				case TriggerParameter::Type::string:
				{
					std::string pre_result;
					if (j.value.size() == 4) {
						if (units_slk.row_header_exists(j.value)) {
							pre_result = units_slk.data("Name", j.value);
						} else if (items_slk.row_header_exists(j.value)) {
							pre_result = items_slk.data("Name", j.value);
						} else {
							pre_result = j.value;
						}
					}

					if (pre_result.size() > 8 && pre_result.substr(0, 7) == "TRIGSTR") {
						result += map->trigger_strings.string(pre_result);
					} else if (!pre_result.empty()) {
						result += pre_result;
					} else if (j.value.size() > 8 && j.value.substr(0, 7) == "TRIGSTR") {
						result += map->trigger_strings.string(j.value);
					} else {
						result += j.value;
					}
					break;
				}
				case TriggerParameter::Type::variable:
				{
					if (j.value.size() > 7 && j.value.substr(0, 7) == "gg_unit") {
						std::string type = j.value.substr(8, 4);
						std::string instance = j.value.substr(13);
						result += units_slk.data("Name", type);
						result += " " + instance;
					} else {
						std::string type = map->triggers.variables[j.value].type;
						if (type == "unit") {
							//std::cout << "test\n";
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
	}

	return result;
}