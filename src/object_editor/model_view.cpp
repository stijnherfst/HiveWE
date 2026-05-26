#include "model_view.h"

#include "model_editor.h"
#include "model_grid_glwidget.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

import std;
import Hierarchy;
import Globals;
import WindowHandler;
import Utilities;

namespace fs = std::filesystem;

namespace {
	struct GameCategorySource {
		ModelCategory category;
		const char* glob;
		const char* label;
	};

	constexpr GameCategorySource k_game_sources[] = {
		{ModelCategory::Abilities, "war3.w3mod:abilities/*.mdx", "Abilities"},
		{ModelCategory::Buildings, "war3.w3mod:buildings/*.mdx", "Buildings"},
		{ModelCategory::Doodads, "war3.w3mod:doodads/*.mdx", "Doodads"},
		{ModelCategory::Environment, "war3.w3mod:environment/*.mdx", "Environment"},
		{ModelCategory::Objects, "war3.w3mod:objects/*.mdx", "Objects"},
		{ModelCategory::SharedModels, "war3.w3mod:sharedmodels/*.mdx", "Shared Models"},
		{ModelCategory::Units, "war3.w3mod:units/*.mdx", "Units"},
	};

	constexpr std::string_view k_war3_prefix = "war3.w3mod:";
} // namespace

ModelView::ModelView(QWidget* parent) : QWidget(parent) {
	std::vector<ModelEntry> entries;
	std::unordered_set<std::string> map_keys;

	if (!hierarchy.map_directory.empty() && fs::exists(hierarchy.map_directory)) {
		std::error_code ec;
		for (const auto& it : fs::recursive_directory_iterator(hierarchy.map_directory, ec)) {
			if (ec) {
				break;
			}
			if (!it.is_regular_file()) {
				continue;
			}
			const auto& p = it.path();
			if (p.extension() != ".mdx") {
				continue;
			}
			const auto stem = p.stem().string();
			if (stem.ends_with("_portrait")) {
				continue;
			}
			std::string rel = p.lexically_relative(hierarchy.map_directory).string();
			normalize_path_to_forward_slash(rel);

			map_keys.insert(to_lowercase_copy(rel));
			entries.push_back(ModelEntry {fs::path(std::move(rel)), ModelCategory::Map});
		}
	}

	for (const auto& src : k_game_sources) {
		auto names = hierarchy.game_data.find_files(src.glob);
		for (auto& name : names) {
			if (name.starts_with(k_war3_prefix)) {
				name.erase(0, k_war3_prefix.size());
			}

			std::string key = name;
			normalize_path_to_forward_slash(key);
			if (map_keys.contains(key)) {
				continue;
			}
			fs::path p(name);
			const auto stem = p.stem().string();
			if (stem.ends_with("_portrait")) {
				continue;
			}
			entries.push_back(ModelEntry {std::move(p), src.category});
		}
	}

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(search);

	QHBoxLayout* category_row = new QHBoxLayout;
	category_row->setContentsMargins(0, 0, 0, 0);
	std::vector<QCheckBox*> category_boxes(static_cast<size_t>(ModelCategory::Count), nullptr);
	for (const auto& src : k_game_sources) {
		auto* cb = new QCheckBox(src.label);
		cb->setChecked(true);
		category_boxes[static_cast<size_t>(src.category)] = cb;
		category_row->addWidget(cb);
	}

	{
		auto* cb = new QCheckBox("Map");
		cb->setChecked(true);
		category_boxes[static_cast<size_t>(ModelCategory::Map)] = cb;
		category_row->addWidget(cb);
	}
	category_row->addStretch(1);
	layout->addLayout(category_row);

	ModelGridGLWidget* grid = new ModelGridGLWidget(entries, this);
	QScrollBar* bar = new QScrollBar(Qt::Vertical, this);

	QHBoxLayout* grid_row = new QHBoxLayout;
	grid_row->setContentsMargins(0, 0, 0, 0);
	grid_row->setSpacing(0);
	grid_row->addWidget(grid, 1);
	grid_row->addWidget(bar);
	layout->addLayout(grid_row, 1);

	connect(bar, &QScrollBar::valueChanged, grid, &ModelGridGLWidget::set_scroll_offset);
	connect(grid, &ModelGridGLWidget::scroll_changed, bar, &QScrollBar::setValue);
	connect(grid, &ModelGridGLWidget::content_height_changed, this, [bar, grid](int total) {
		bar->setRange(0, std::max(0, total - grid->height()));
		bar->setPageStep(grid->height());
		bar->setSingleStep(grid->cell_pixel_size());
	});
	connect(grid, &ModelGridGLWidget::clicked, this, [this](const fs::path& p) {
		finalPath->setText(QString::fromStdString(p.string()));
	});

	connect(grid, &ModelGridGLWidget::double_clicked, this, [this](const fs::path& p) {
		emit doubleClicked(p);
	});

	connect(search, &QLineEdit::textChanged, grid, &ModelGridGLWidget::set_search);

	auto update_categories = [grid, category_boxes]() {
		std::bitset<static_cast<size_t>(ModelCategory::Count)> mask;
		for (size_t i = 0; i < category_boxes.size(); ++i) {
			if (category_boxes[i] && category_boxes[i]->isChecked()) {
				mask.set(i);
			}
		}
		grid->set_categories(mask);
	};
	for (auto* cb : category_boxes) {
		if (cb) {
			connect(cb, &QCheckBox::toggled, this, [update_categories](bool) {
				update_categories();
			});
		}
	}

	open_in_model_editor->setIcon(QIcon("data/icons/ribbon/model_editor.png"));
	open_in_model_editor->setIconSize(QSize(20, 20));
	open_in_model_editor->setToolTip("Open in Model Editor");

	QHBoxLayout* hlayout = new QHBoxLayout;
	hlayout->addWidget(new QLabel("Path"));
	hlayout->addWidget(finalPath);
	hlayout->addWidget(open_in_model_editor);
	layout->addLayout(hlayout);
	setLayout(layout);

	search->setPlaceholderText("Search Models");

	connect(open_in_model_editor, &QPushButton::clicked, [this] {
		bool created = false;
		const auto model_editor = window_handler.create_or_raise<ModelEditor>(nullptr, created);

		const auto opened = model_editor->open_model(finalPath->text().toStdString(), false);
		if (!opened) {
			QMessageBox::critical(
				this,
				"Error opening model",
				QString::fromStdString(std::format("Failed to open model with: {}", opened.error()))
			);
		}
	});
}

QString ModelView::current_model_path() {
	return finalPath->text();
}

void ModelView::set_current_model_path(const QString& path) {
	finalPath->setText(path);
}
