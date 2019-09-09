#include "TriggerEditor.h"

#include <QPlainTextEdit>
#include <QSettings>
#include <QSplitter>
#include <QMessageBox>
#include <QMap>
#include <QPainter>
#include <QLineEdit>
#include <QPushButton>
#include <QFileIconProvider>

#include "Utilities.h"
#include "HiveWE.h"
#include "Triggers.h"
#include "JassEditor.h"
#include "SearchWindow.h"
#include <QTreeWidget>

TriggerEditor::TriggerEditor(QWidget* parent) : QMainWindow(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	model = new TreeModel(explorer);
	explorer->setModel(model);

	setCentralWidget(dock_manager);

	ads::CDockWidget* explorer_widget = new ads::CDockWidget("Trigger Explorer");
	explorer_widget->setObjectName("-1");
	explorer_widget->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	explorer_widget->setWidget(explorer);
	dock_manager->addDockWidget(ads::LeftDockWidgetArea, explorer_widget);
	dock_manager->setStyleSheet("");

	show();

	QFileIconProvider icons;
	gui_icon = icons.icon(QFileIconProvider::File);
	script_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerScript") + ".blp");
	script_icon_disabled = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerScriptDisable") + ".blp");
	variable_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerGlobalVariable") + ".blp");
	comment_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerComment") + ".blp");

	event_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_Event"));
	condition_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_Condition"));
	action_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_Action"));

	ui.actionCreateGuiTrigger->setIcon(gui_icon);
	ui.actionCreateJassTrigger->setIcon(script_icon);
	ui.actionCreateComment->setIcon(comment_icon);

	connect(ui.actionGenerateScript, &QAction::triggered, [&]() {
		save_changes();
		map->triggers.generate_map_script();
	});
	connect(ui.actionCreateJassTrigger, &QAction::triggered, explorer, &TriggerExplorer::createJassTrigger);
	connect(ui.actionCreateComment, &QAction::triggered, explorer, &TriggerExplorer::createComment);

	connect(explorer, &QTreeView::doubleClicked, this, &TriggerEditor::item_clicked);
	connect(explorer, &TriggerExplorer::itemAboutToBeDeleted, [&](TreeItem* item) {
		if (auto found = dock_manager->findDockWidget(QString::number(item->id)); found) {
			found->close();
			if (dock_area->dockWidgets().contains(found) && dock_area->dockWidgetsCount() == 1) {
				dock_area = nullptr;
			}
			dock_manager->removeDockWidget(found);
			found->deleteLater();
		}
	});
	connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), this), &QShortcut::activated, this, &TriggerEditor::focus_search_window);
}

void TriggerEditor::focus_search_window() {
	SearchWindow* search = new SearchWindow(this);
	search->move(500, 10);

	connect(search, &SearchWindow::text_changed, [&](QString text) {
		for (const auto& tab : dock_manager->dockWidgetsMap()) {
			int trigger_id = tab->objectName().toInt();
			if (trigger_id < 0) {
				continue;
			}

			auto editor = tab->findChild<JassEditor*>("jass_editor");
			if (editor) {
				editor->highlight_text(text.toStdString());
			}
		}
	});
}

void TriggerEditor::item_clicked(const QModelIndex& index) {
	TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
	if (item->type == Classifier::category || item->type == Classifier::comment) {
		return;
	}

	QSettings settings;
	bool comments = settings.value("comments").toString() != "False";

	if (item->id == 0) { // Map header
		if (auto found = dock_manager->findDockWidget("0"); found) {
			found->dockAreaWidget()->setCurrentDockWidget(found);
			return;
		}

		JassEditor* jass_editor = new JassEditor;
		jass_editor->setObjectName("jass_editor");
		jass_editor->setText(QString::fromStdString(map->triggers.global_jass));

		ads::CDockWidget* dock_tab = new ads::CDockWidget("Map Header");
		dock_tab->setObjectName("0");
		dock_tab->setIcon(model->data(index, Qt::DecorationRole).value<QIcon>());
		if (dock_area == nullptr) {
			dock_area = dock_manager->addDockWidget(ads::RightDockWidgetArea, dock_tab, dock_area);
		} else {
			dock_manager->addDockWidget(ads::CenterDockWidgetArea, dock_tab, dock_area);
		}
		
		connect(dock_tab, &ads::CDockWidget::closed, [&, dock_tab, item]() {
			map->triggers.global_jass = dock_tab->findChild<JassEditor*>("jass_editor")->text().toStdString();

			auto comments = dock_tab->findChild<QPlainTextEdit*>("comments");
			if (comments) {
				map->triggers.global_jass_comment = comments->toPlainText().toStdString();
			}
			if (dock_area->dockWidgets().contains(dock_tab) && dock_area->dockWidgetsCount() == 1) {
				dock_area = nullptr;
			}

			dock_manager->removeDockWidget(dock_tab);
		});

		QSplitter* splitter = new QSplitter(Qt::Orientation::Vertical);

		if (comments) {
			QPlainTextEdit* comments_editor = new QPlainTextEdit;
			comments_editor->setObjectName("comments");
			comments_editor->setPlaceholderText("Optional comments here");
			splitter->addWidget(comments_editor);
			comments_editor->setPlainText(QString::fromStdString(map->triggers.global_jass_comment));
		}

		splitter->addWidget(jass_editor);
		splitter->setStretchFactor(0, 1);
		splitter->setStretchFactor(1, 7);
		dock_tab->setWidget(splitter);

		return;
	}
	
	Trigger& trigger = *std::find_if(map->triggers.triggers.begin(), map->triggers.triggers.end(), [item](const Trigger& trigger) {
		return trigger.id == item->id;
	});

	// Check if trigger is already open and if so focus it
	if (auto found = dock_manager->findDockWidget(QString::number(trigger.id)); found) {
		found->dockAreaWidget()->setCurrentDockWidget(found);
		return;
	}

	QSplitter* splitter = new QSplitter(Qt::Orientation::Vertical);
	if (comments) {
		QPlainTextEdit* comments_editor = new QPlainTextEdit;
		comments_editor->setObjectName("comments");
		comments_editor->setPlaceholderText("Optional comments here");
		comments_editor->setPlainText(QString::fromStdString(trigger.description));
		splitter->addWidget(comments_editor);
	}

	ads::CDockWidget* dock_tab = new ads::CDockWidget(QString::fromStdString(trigger.name));
	dock_tab->setObjectName(QString::number(trigger.id));
	dock_tab->setIcon(model->data(index, Qt::DecorationRole).value<QIcon>());

	if (!trigger.is_comment) {
		if (trigger.is_script) {
			JassEditor* edit = new JassEditor;
			edit->setObjectName("jass_editor");
			splitter->addWidget(edit);
			edit->setText(QString::fromStdString(trigger.custom_text));
		} else {
			QTreeWidget* edit = new QTreeWidget;
			edit->setHeaderHidden(true);
			edit->setUniformRowHeights(true);
			splitter->addWidget(edit);
			show_gui_trigger(edit, trigger);
			edit->expandAll();
		}
		connect(dock_tab, &ads::CDockWidget::closed, [&, dock_tab, item]() {
			for (int i = 0; i < map->triggers.triggers.size(); i++) {
				Trigger& trigger = map->triggers.triggers[i];
				if (trigger.id == item->id) {
					auto editor = dock_tab->findChild<JassEditor*>("jass_editor");
					auto comments = dock_tab->findChild<QPlainTextEdit*>("comments");
					if (editor) {
						trigger.custom_text = editor->text().toStdString();
					}
					if (comments) {
						trigger.description = comments->toPlainText().toStdString();
					}
					if (dock_area->dockWidgets().contains(dock_tab) && dock_area->dockWidgetsCount() == 1) {
						dock_area = nullptr;
					}
					dock_manager->removeDockWidget(dock_tab);

					std::cout << trigger.name << "\n";
					break;
				}
			}
		});
	}

	if (dock_area == nullptr) {
		dock_area = dock_manager->addDockWidget(ads::RightDockWidgetArea, dock_tab, dock_area);
	} else {
		dock_manager->addDockWidget(ads::CenterDockWidgetArea, dock_tab, dock_area);
	}

	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 7);
	dock_tab->setWidget(splitter);
}

void TriggerEditor::show_gui_trigger(QTreeWidget* edit, const Trigger& trigger) {
	QTreeWidgetItem* events = new QTreeWidgetItem(edit);
	events->setText(0, "Events");
	events->setIcon(0, event_icon);

	QTreeWidgetItem* conditions = new QTreeWidgetItem(edit);
	conditions->setText(0, "Conditions");
	conditions->setIcon(0, condition_icon);

	QTreeWidgetItem* actions = new QTreeWidgetItem(edit);
	actions->setText(0, "Actions");
	actions->setIcon(0, action_icon);

	std::function<void(QTreeWidgetItem*, const ECA&)> recurse = [&](QTreeWidgetItem* parent, const ECA& i) {
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

		for (const auto& j : i.ecas) {
			if (j.group == 0) {
				recurse(sub1, j);
			} else if (j.group == 1) {
				recurse(sub2, j);
			} else if (j.group == 2) {
				recurse(sub3, j);
			}
		}
	};

	for (const auto& i : trigger.ecas) {
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
	for (const auto& i : string_parameters) {
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
				case TriggerParameter::Type::string: {
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
				case TriggerParameter::Type::variable: {
					if (j.value.size() > 7 && j.value.substr(0, 7) == "gg_unit") {
						std::string type = j.value.substr(8, 4);
						std::string instance = j.value.substr(13);
						result += units_slk.data("Name", type);
						result += " " + instance;
					} else {
						//std::string type = map->triggers.variables[j.value].type;
						//if (type == "unit") {
							//std::cout << "test\n";
						//} else {
							result += j.value;
						//}
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

void TriggerEditor::save_changes() {
	QSettings settings;
	bool comments = settings.value("comments").toString() != "False";
	for (const auto& tab : dock_manager->dockWidgetsMap()) {

		int trigger_id = tab->objectName().toInt();
		if (trigger_id < 0) {
			continue;
		}

		if (trigger_id == 0) {
			std::cout << "saved map header\n";
			auto t = tab->objectName();
			auto editor = tab->findChild<JassEditor*>("jass_editor");
			if (comments) {
				map->triggers.global_jass_comment = tab->findChild<QPlainTextEdit*>("comments")->toPlainText().toStdString();
			}
			map->triggers.global_jass = editor->text().toStdString();
		} else {
			for (auto& trigger : map->triggers.triggers) {
				if (trigger.id == trigger_id) {
					if (comments) {
						trigger.description = tab->findChild<QPlainTextEdit*>("comments")->toPlainText().toStdString();
					}
					JassEditor* editor = tab->findChild<JassEditor*>("jass_editor");
					if (editor) {
						std::cout << "saved trigger: " << trigger_id << "\n";

						trigger.custom_text = editor->text().toStdString();
					}
					break;
				}
			}
		}
	}
}