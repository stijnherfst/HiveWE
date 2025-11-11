#include "trigger_editor.h"

#include <QPlainTextEdit>
#include <QSettings>
#include <QSplitter>
#include <QMessageBox>
#include <QMap>
#include <QPainter>
#include <QLineEdit>
#include <QPushButton>
#include <QFileIconProvider>
#include <QTreeWidget>

#include "HiveWE.h"
#include "jass_editor.h"
#include "search_window.h"
#include "variable_editor.h"
#include "trigger_model.h"

import std;
import Utilities;
import Triggers;
import Globals;
import OpenGLUtilities;
import MapGlobal;

constexpr int map_header_id = 0;

TriggerEditor::TriggerEditor(QWidget* parent) : QMainWindow(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	model = new TreeModel(explorer);
	explorer->setModel(model);
	explorer->expandToDepth(1);

	compile_output->setReadOnly(true);
	dock_manager = new ads::CDockManager;
	dock_manager->setStyleSheet("");
	setCentralWidget(dock_manager);

	QLabel* image = new QLabel();
	image->setPixmap(QPixmap("data/icons/trigger_editor/background.png"));
	image->setAlignment(Qt::AlignCenter);

	ads::CDockWidget* centraldock_widget = new ads::CDockWidget(dock_manager, "");
	centraldock_widget->setWidget(image);
	centraldock_widget->setObjectName("-1");
	centraldock_widget->setFeature(ads::CDockWidget::NoTab, true);
	dock_area = dock_manager->setCentralWidget(centraldock_widget);

	ads::CDockWidget* explorer_widget = new ads::CDockWidget(dock_manager, "Trigger Explorer");
	explorer_widget->setObjectName("-1");
	explorer_widget->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	explorer_widget->setWidget(explorer);
	dock_manager->addDockWidget(ads::LeftDockWidgetArea, explorer_widget, dock_area);

	ads::CDockWidget* output_widget = new ads::CDockWidget(dock_manager, "Output");
	output_widget->setObjectName("-11");
	output_widget->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	output_widget->setWidget(compile_output);
	dock_manager->addDockWidget(ads::BottomDockWidgetArea, output_widget, dock_area);

	show();

	QFileIconProvider icons;
	gui_icon = icons.icon(QFileIconProvider::File);
	script_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerScript") + ".dds");
	script_icon_disabled = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerScriptDisable") + ".dds");
	variable_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerGlobalVariable") + ".dds");
	comment_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerComment") + ".dds");

	event_icon = texture_to_icon(string_replaced(world_edit_data.data("WorldEditArt", "SEIcon_Event"), "blp", "dds"));
	condition_icon = texture_to_icon(string_replaced(world_edit_data.data("WorldEditArt", "SEIcon_Condition"), "blp", "dds"));
	action_icon = texture_to_icon(string_replaced(world_edit_data.data("WorldEditArt", "SEIcon_Action"), "blp", "dds"));

	ui.actionCreateCategory->setIcon(icons.icon(QFileIconProvider::Folder));
	ui.actionCreateGuiTrigger->setIcon(gui_icon);
	ui.actionCreateJassTrigger->setIcon(script_icon);
	ui.actionCreateVariable->setIcon(variable_icon);
	ui.actionCreateComment->setIcon(comment_icon);

	connect(ui.actionGenerateScript, &QAction::triggered, [&]() {
		save_changes();

		ScriptMode mode = ScriptMode::jass;
		if (map->info.lua) {
			mode = ScriptMode::lua;
		}
		const auto result =
			map->triggers
				.generate_map_script(map->terrain, map->units, map->doodads, map->info, map->sounds, map->regions, map->cameras, mode);

		if (!result) {
			compile_output->setPlainText(QString::fromStdString(result.error()));
		}
	});
	connect(ui.actionCreateCategory, &QAction::triggered, explorer, &TriggerExplorer::createCategory);
	connect(ui.actionCreateGuiTrigger, &QAction::triggered, explorer, &TriggerExplorer::createGuiTrigger);
	connect(ui.actionCreateJassTrigger, &QAction::triggered, explorer, &TriggerExplorer::createJassTrigger);
	connect(ui.actionCreateVariable, &QAction::triggered, explorer, &TriggerExplorer::createVariable);
	connect(ui.actionCreateComment, &QAction::triggered, explorer, &TriggerExplorer::createComment);

	connect(explorer, &QTreeView::doubleClicked, this, &TriggerEditor::item_clicked);
	connect(explorer, &TriggerExplorer::itemAboutToBeDeleted, [&](TreeItem* item) {
		if (auto found = dock_manager->findDockWidget(QString::number(item->id)); found) {
			found->closeDockWidget();
		}
	});
	connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this), &QShortcut::activated, this, &TriggerEditor::focus_search_window);
}

TriggerEditor::~TriggerEditor() {
	save_changes();
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
	const TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
	if (item->type == Classifier::category) {
		return;
	}

	if (auto found = dock_manager->findDockWidget(QString::number(item->id)); found) {
		found->dockAreaWidget()->setCurrentDockWidget(found);
		found->setFocus();
		found->raise();
		return;
	}

	ads::CDockWidget* dock_tab = new ads::CDockWidget(dock_manager, "");
	dock_tab->setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetDeleteOnClose, true);

	if (item->type == Classifier::gui || item->type == Classifier::script) {
		QSettings settings;
		bool comments_enabled = settings.value("comments").toString() != "False";

		QSplitter* splitter = new QSplitter(Qt::Orientation::Vertical);

		if (item->id == 0) { // Map header
			JassEditor* jass_editor = new JassEditor;
			jass_editor->setObjectName("jass_editor");
			jass_editor->setText(QString::fromStdString(map->triggers.global_jass));

			dock_tab->setWindowTitle("Map Header");
			dock_tab->setObjectName("0");
			dock_tab->setIcon(model->data(index, Qt::DecorationRole).value<QIcon>());

			if (comments_enabled) {
				QPlainTextEdit* comments_editor = new QPlainTextEdit;
				comments_editor->setObjectName("comments");
				comments_editor->setPlaceholderText("Optional comments here");
				splitter->addWidget(comments_editor);
				comments_editor->setPlainText(QString::fromStdString(map->triggers.global_jass_comment));
			}

			splitter->addWidget(jass_editor);
		} else {
			const Trigger& trigger = *std::ranges::find_if(map->triggers.triggers, [item](const Trigger& trigger) {
				return trigger.id == item->id;
			});

			if (comments_enabled) {
				QPlainTextEdit* comments_editor = new QPlainTextEdit;
				comments_editor->setObjectName("comments");
				comments_editor->setPlaceholderText("Optional comments here");
				comments_editor->setPlainText(QString::fromStdString(trigger.description));
				splitter->addWidget(comments_editor);
			}

			dock_tab->setWindowTitle(QString::fromStdString(trigger.name));
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
			}
		}

		splitter->setStretchFactor(0, 1);
		splitter->setStretchFactor(1, 7);
		dock_tab->setWidget(splitter);
	} else if (item->type == Classifier::variable) {
		TriggerVariable& variable =
			*std::ranges::find_if(map->triggers.variables, [item](const TriggerVariable& i) {
				return i.id == item->id;
			});

		dock_tab->setWindowTitle(QString::fromStdString(variable.name));
		dock_tab->setObjectName(QString::number(variable.id));
		VariableEditor* edit = new VariableEditor(variable);
		edit->setObjectName("var_editor");
		dock_tab->setWidget(edit);
	} else if (item->type == Classifier::comment) {
		Trigger& trigger = *std::ranges::find_if(map->triggers.triggers, [item](const Trigger& trigger) {
			return trigger.id == item->id;
		});

		QPlainTextEdit* comments_editor = new QPlainTextEdit;
		comments_editor->setObjectName("comments");
		comments_editor->setPlaceholderText("Optional comments here");
		comments_editor->setPlainText(QString::fromStdString(trigger.description));

		dock_tab->setWindowTitle(QString::fromStdString(trigger.name));
		dock_tab->setObjectName(QString::number(trigger.id));
		dock_tab->setIcon(model->data(index, Qt::DecorationRole).value<QIcon>());
		dock_tab->setWidget(comments_editor);
	}

	connect(dock_tab, &ads::CDockWidget::closeRequested, [&, dock_tab]() {
		save_tab(dock_tab);
	});

	dock_manager->addDockWidget(ads::CenterDockWidgetArea, dock_tab, dock_area);
}

void TriggerEditor::save_tab(ads::CDockWidget* tab) {
	const int trigger_id = tab->objectName().toInt();

	if (trigger_id < 0) {
		return;
	}

	// Comments
	auto comments = tab->findChild<QPlainTextEdit*>("comments");
	if (comments) {
		if (trigger_id == map_header_id) {
			map->triggers.global_jass_comment = comments->toPlainText().toStdString();
		} else {
			Trigger& trigger =
				*std::ranges::find_if(map->triggers.triggers, [trigger_id](const Trigger& trigger) {
					return trigger.id == trigger_id;
				});

			trigger.description = comments->toPlainText().toStdString();
		}
	}

	// Jass editor
	auto jass_editor = tab->findChild<JassEditor*>("jass_editor");
	if (jass_editor) {
		if (trigger_id == map_header_id) {
			map->triggers.global_jass = jass_editor->text().toStdString();
		} else {
			Trigger& trigger =
				*std::ranges::find_if(map->triggers.triggers, [trigger_id](const Trigger& trigger) {
					return trigger.id == trigger_id;
				});

			trigger.custom_text = jass_editor->text().toStdString();
		}
	}

	// Variable editor
	auto var_editor = tab->findChild<VariableEditor*>("var_editor");
	if (var_editor) {
		TriggerVariable& variable =
			*std::ranges::find_if(map->triggers.variables, [trigger_id](const TriggerVariable& i) {
				return i.id == trigger_id;
			});

		variable.name = var_editor->ui.name->text().toStdString();
		variable.type = var_editor->ui.type->text().toStdString();
		variable.is_array = var_editor->ui.array->isChecked();
		variable.array_size = var_editor->ui.array_size->value();
		variable.is_initialized = !var_editor->ui.value->text().isEmpty();
		variable.initial_value = var_editor->ui.value->text().toStdString();
	}
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
				string_parameters = map->triggers.trigger_data.whole_data("TriggerEvents", "_" + i.name + "_Parameters");
				category = map->triggers.trigger_data.data("TriggerEvents", "_" + i.name + "_Category");
				break;
			case ECA::Type::condition:
				string_parameters = map->triggers.trigger_data.whole_data("TriggerConditions", "_" + i.name + "_Parameters");
				category = map->triggers.trigger_data.data("TriggerConditions", "_" + i.name + "_Category");
				break;
			case ECA::Type::action:
				string_parameters = map->triggers.trigger_data.whole_data("TriggerActions", "_" + i.name + "_Parameters");
				category = map->triggers.trigger_data.data("TriggerActions", "_" + i.name + "_Category");
				break;
		}

		eca->setText(0, QString::fromStdString(get_parameters_names(string_parameters, i.parameters)));

		if (auto found = trigger_icons.find(category); found == trigger_icons.end()) {
			const std::string icon_path = map->triggers.trigger_data.data("TriggerCategories", category, 1);
			const std::string final_path = icon_path + ".dds";
			const QIcon icon = texture_to_icon(final_path);
			trigger_icons[category] = icon;
			eca->setIcon(0, icon);
		} else {
			eca->setIcon(0, found->second);
		}

		QTreeWidgetItem* sub1 = eca;
		QTreeWidgetItem* sub2 = eca;
		QTreeWidgetItem* sub3 = eca;

		if (i.name == "AndMultiple" || i.name == "OrMultiple") {
			sub1 = new QTreeWidgetItem(eca, {"Conditions"});
			sub1->setIcon(0, condition_icon);
		} else if (i.name == "IfThenElseMultiple") {
			sub1 = new QTreeWidgetItem(eca, {"If - Conditions"});
			sub1->setIcon(0, condition_icon);
			sub2 = new QTreeWidgetItem(eca, {"Then - Actions"});
			sub2->setIcon(0, action_icon);
			sub3 = new QTreeWidgetItem(eca, {"Else - Actions"});
			sub3->setIcon(0, action_icon);
		} else if (i.name == "ForLoopAMultiple" || i.name == "ForLoopBMultiple" || i.name == "ForLoopVarMultiple" || i.name == "ForLoopA"
				   || i.name == "ForLoopB" || i.name == "ForLoopVar" || i.name == "ForForceMultiple" || i.name == "ForGroupMultiple") {
			sub1 = new QTreeWidgetItem(eca, {"Loop - Actions"});
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

std::string TriggerEditor::get_parameters_names(
	const std::vector<std::string>& string_parameters,
	const std::vector<TriggerParameter>& parameters
) const {
	std::string result;

	size_t current_parameter = 0;
	for (const auto& i : string_parameters) {
		if (i.empty() || i.front() != '~') {
			result += i;
			continue;
		}
		const TriggerParameter& j = parameters.at(current_parameter);

		if (j.has_sub_parameter) {
			std::vector<std::string> sub_string_parameters;
			switch (j.sub_parameter.type) {
				case ECA::Type::event:
					sub_string_parameters =
						map->triggers.trigger_data.whole_data("TriggerEvents", "_" + j.sub_parameter.name + "_Parameters");
					break;
				case ECA::Type::condition:
					sub_string_parameters =
						map->triggers.trigger_data.whole_data("TriggerConditions", "_" + j.sub_parameter.name + "_Parameters");
					break;
				case ECA::Type::action:
					sub_string_parameters =
						map->triggers.trigger_data.whole_data("TriggerActions", "_" + j.sub_parameter.name + "_Parameters");
					break;
				case ECA::Type::call:
					sub_string_parameters =
						map->triggers.trigger_data.whole_data("TriggerCalls", "_" + j.sub_parameter.name + "_Parameters");
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
						if (units_slk.row_headers.contains(j.value)) {
							pre_result = units_slk.data("name", j.value);
						} else if (items_slk.row_headers.contains(j.value)) {
							pre_result = items_slk.data("name", j.value);
						} else {
							pre_result = j.value;
						}
					}

					if (pre_result.starts_with("TRIGSTR")) {
						result += map->trigger_strings.string(pre_result);
					} else if (!pre_result.empty()) {
						result += pre_result;
					} else if (j.value.starts_with("TRIGSTR")) {
						result += map->trigger_strings.string(j.value);
					} else {
						result += j.value;
					}
					break;
				}
				case TriggerParameter::Type::variable: {
					if (j.value.starts_with("gg_unit")) {
						std::string type = j.value.substr(8, 4);
						std::string instance = j.value.substr(13);
						result += units_slk.data("name", type);
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
	for (const auto& tab : dock_manager->dockWidgetsMap()) {
		save_tab(tab);
	}
}
