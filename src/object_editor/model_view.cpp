#include "model_view.h"

#include "model_editor.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QListView>
#include <QLabel>
#include <QMessageBox>

import std;
import Hierarchy;
import Globals;
import WindowHandler;

namespace fs = std::filesystem;

ModelView::ModelView(QWidget* parent) : QWidget(parent) {
	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	//layout->addWidget(type);
	layout->addWidget(search);
	layout->addWidget(new QLabel("Coming soon: a grid view of models here"));
	// layout->addWidget(view);

	open_in_model_editor->setIcon(QIcon("data/icons/ribbon/model_editor.png"));
	open_in_model_editor->setIconSize(QSize(20, 20));
	open_in_model_editor->setToolTip("Open in Model Editor");

	QHBoxLayout* hlayout = new QHBoxLayout;
	hlayout->addWidget(new QLabel("Path"));
	hlayout->addWidget(finalPath);
	hlayout->addWidget(open_in_model_editor);
	//layout->addWidget(finalPath);
	layout->addLayout(hlayout);
	setLayout(layout);

	type->addItem("Units");
	type->addItem("Items");
	type->addItem("Abilities");
	type->addItem("Upgrades");
	type->addItem("Buffs");
	search->setPlaceholderText("Search Models");

	connect(open_in_model_editor, &QPushButton::clicked, [this] {
		bool created = false;
		auto model_editor = window_handler.create_or_raise<ModelEditor>(nullptr, created);

		auto path = fs::path(finalPath->text().toStdString());
		path.replace_extension(".mdx");

		const auto opened = model_editor->open_model(path, false);
		if (!opened) {
			QMessageBox::critical(this, "Error opening model", QString::fromStdString(std::format("Failed to open model with: {}", opened.error())));
		}
	});
}

QString ModelView::current_model_path() {
	return finalPath->text();
}

void ModelView::set_current_model_path(const QString& path) {
	finalPath->setText(path);
}
