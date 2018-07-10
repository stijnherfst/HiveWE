#include "stdafx.h"

TriggerEditor::TriggerEditor(QWidget* parent) : QMainWindow(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	ui.splitter->setSizes({ 10000, 20000 });


	show();

	QFileIconProvider icons;
	folder_icon = icons.icon(QFileIconProvider::Folder);
	file_icon = icons.icon(QFileIconProvider::File);

	for (auto&& i : map.triggers.categories) {
		QTreeWidgetItem* item = new QTreeWidgetItem(ui.explorer);
		item->setData(0, Qt::EditRole, QString::fromStdString(i.name));
		item->setIcon(0, folder_icon);
		folders[i.id] = item;
	}

	for (auto&& i : map.triggers.triggers) {
		QTreeWidgetItem* item = new QTreeWidgetItem(folders[i.category_id]);
		item->setData(0, Qt::EditRole, QString::fromStdString(i.name));
		item->setIcon(0, file_icon);
		files.emplace(item, i);
	}

	connect(ui.explorer, &QTreeWidget::itemClicked, this, &TriggerEditor::item_clicked);
}

void TriggerEditor::item_clicked(QTreeWidgetItem* item) {
	if (files.find(item) == files.end()) {
		return;
	}
	Trigger& trigger = files.at(item).get();
	
	QWidget* tab = new QWidget;
	QVBoxLayout* layout = new QVBoxLayout(tab);

	if (!trigger.custom_text.empty()) {
		JassEditor* edit = new JassEditor;
		layout->addWidget(edit);
		edit->setText(QString::fromStdString(trigger.custom_text));
	}

	ui.editor->addTab(tab, QString::fromStdString(trigger.name)); 
}
