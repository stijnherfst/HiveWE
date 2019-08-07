#include "TriggerEditor.h"

#include <QPlainTextEdit>
#include <QFileIconProvider>
#include "Utilities.h"
#include "HiveWE.h"
#include "Triggers.h"
#include "JassEditor.h"

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

	QTreeWidgetItem* root = new QTreeWidgetItem(ui.explorer);
	root->setData(0, Qt::EditRole, QString::fromStdString("Map Header"));
	root->setIcon(0, folder_icon); //maybe use a special icon
	root->setExpanded(true);
	folders[0] = root;

	for (const auto& i : map->triggers.categories) {
		QTreeWidgetItem* item  = new QTreeWidgetItem(folders[i.parent_id]);
		item->setData(0, Qt::EditRole, QString::fromStdString(i.name));
		item->setIcon(0, folder_icon);
		folders[i.id] = item;
	}

	for (auto&& i : map->triggers.triggers) {
		QTreeWidgetItem* item = new QTreeWidgetItem(folders[i.parent_id]);
		item->setData(0, Qt::EditRole, QString::fromStdString(i.name));
		if (i.is_comment) {
			item->setIcon(0, trigger_comment_icon);
		} else if (i.classifier == Classifier::script) {
			item->setIcon(0, file_icon); //maybe give scripts a different icon
		}
		else {
			item->setIcon(0, file_icon);
		}
		
		files.emplace(item, i);
	}

	for (auto&& i : map->triggers.variables) {
		QTreeWidgetItem* item = new QTreeWidgetItem(folders[i.second.parent_id]);
		item->setData(0, Qt::EditRole, QString::fromStdString(i.first));
		item->setIcon(0, file_icon);
	}
	
	connect(ui.explorer, &QTreeWidget::itemDoubleClicked, this, &TriggerEditor::item_clicked);
	connect(ui.editor, &QTabWidget::tabCloseRequested, [&](int index) { delete ui.editor->widget(index); });
	connect(ui.actionGenerateScript, &QAction::triggered, [&]() {
		save_changes();
		map->triggers.generate_map_script();
		});
}

void TriggerEditor::item_clicked(QTreeWidgetItem* item) {
	if (item == folders[0]) {
		// If a tab for this is already open we focus to it
		for (int i = 0; i < ui.editor->count(); i++) {
			if (ui.editor->widget(i)->property("TriggerID").toInt() == 0) {
				ui.editor->setCurrentIndex(i);
				return;
			}
		}

		QWidget* tab = new QWidget;
		tab->setProperty("TriggerID", 0);

		QSplitter* splitter = new QSplitter(Qt::Orientation::Vertical);
		QPlainTextEdit* comments_editor = new QPlainTextEdit;
		comments_editor->setPlaceholderText("Optional comments here");

		JassEditor* jass_editor = new JassEditor;

		QVBoxLayout* layout = new QVBoxLayout(tab);
		splitter->addWidget(comments_editor);
		splitter->addWidget(jass_editor);
		splitter->setStretchFactor(0, 1);
		splitter->setStretchFactor(1, 7);
		layout->setMargin(0);
		layout->addWidget(splitter);

		comments_editor->setPlainText(QString::fromStdString(map->triggers.global_jass_comment));
		jass_editor->setText(QString::fromStdString(map->triggers.global_jass));

		connect(this, &TriggerEditor::save_changes, [=]() {
			map->triggers.global_jass_comment = comments_editor->toPlainText().toStdString();
		});
		connect(this, &TriggerEditor::save_changes, [=]() {
			map->triggers.global_jass = jass_editor->text().toStdString();
		});

		ui.editor->addTab(tab, QString::fromStdString("Map Header"));
		ui.editor->setCurrentWidget(tab);
		return;
	}
	
	if (!files.contains(item)) {
		return;
	}
	Trigger& trigger = files.at(item).get();

	//switch (trigger.classifier) {
	//	case Classifier::trigger:

	//		break;		
	//	case Classifier::script:

	//		break;		
	//	case Classifier::variable:

	//		break;
	//	case Classifier::comment:

	//		break;		
	//	case Classifier::category:

	//		break;		
	//}

	// Check if trigger is already open and if so focus it
	for (int i = 0; i < ui.editor->count(); i++) {
		if (ui.editor->widget(i)->property("TriggerID").toInt() == trigger.id) {
			ui.editor->setCurrentIndex(i);
			return;
		}
	}

	QWidget* tab = new QWidget;
	tab->setProperty("TriggerID", trigger.id);
	QSplitter* splitter = new QSplitter(Qt::Orientation::Vertical);
	QPlainTextEdit* comments_editor = new QPlainTextEdit;
	comments_editor->setPlaceholderText("Optional comments here");

	QVBoxLayout* layout = new QVBoxLayout(tab);
	splitter->addWidget(comments_editor);

	layout->addWidget(splitter);
	layout->setMargin(0);

	connect(this, &TriggerEditor::save_changes, [=]() {
		files.at(item).get().description = comments_editor->toPlainText().toStdString();
	});

	if (!trigger.is_comment) {
		if (trigger.is_script) {
			JassEditor* edit = new JassEditor;
			splitter->addWidget(edit);
			edit->setText(QString::fromStdString(trigger.custom_text));
			connect(this, &TriggerEditor::save_changes, [=]() {
				files.at(item).get().custom_text = edit->text().toStdString();
			});
		} else {
			QTreeWidget* edit = new QTreeWidget;
			edit->setHeaderHidden(true);
			edit->setUniformRowHeights(true);
			splitter->addWidget(edit);
			show_gui_trigger(edit, trigger);
			edit->expandAll();
		}
	}

	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 7);

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