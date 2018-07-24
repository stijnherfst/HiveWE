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

		for (auto&& i : trigger.lines) {
			QTreeWidgetItem* eca = new QTreeWidgetItem;
			std::string category;

			switch (i.type) {
				case ECA::Type::event:
					eca->setText(0, QString::fromStdString(map.triggers.trigger_strings.data("TriggerEventStrings", i.name)));
					events->addChild(eca);
					category = map.triggers.trigger_data.data("TriggerEvents", "_" + i.name + "_Category");
					break;
				case ECA::Type::condition:
					eca->setText(0, QString::fromStdString(map.triggers.trigger_strings.data("TriggerConditionStrings", i.name)));
					conditions->addChild(eca);
					category = map.triggers.trigger_data.data("TriggerConditions", "_" + i.name + "_Category");
					break;
				case ECA::Type::action:
					eca->setText(0, QString::fromStdString(map.triggers.trigger_strings.data("TriggerActionStrings", i.name)));
					actions->addChild(eca);
					category = map.triggers.trigger_data.data("TriggerActions", "_" + i.name + "_Category");
					break;
			}

			std::string icon_path = split(map.triggers.trigger_data.data("TriggerCategories", category), ',')[1];
			std::string final_path = icon_path + ".blp";
			eca->setIcon(0, texture_to_icon("ReplaceableTextures\\WorldEditUI\\Actions-Nothing.blp"));
		}
	}

	ui.editor->addTab(tab, QString::fromStdString(trigger.name));
	ui.editor->setCurrentWidget(tab);
}