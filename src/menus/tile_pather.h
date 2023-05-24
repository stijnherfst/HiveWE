#pragma once

#include "ui_tile_pather.h"

#include <QWidget>
#include <QDialog>
#include <QButtonGroup>
#include <QAbstractButton>

import FlowLayout;

struct PathingOptions {
	bool unwalkable = false;
	bool unflyable = false;
	bool unbuildable = false;

	enum class Operation {
		replace,
		add,
		remove
	};
	Operation operation = Operation::replace;

	bool apply_retroactively = false;
};

class TilePather : public QDialog {
	Q_OBJECT

public:
	explicit TilePather(QWidget* parent = nullptr);

private:
	void changed_tile(QAbstractButton* button);
	void save_tiles();

	Ui::TilePather ui;
	QButtonGroup* selected_group = new QButtonGroup;
	FlowLayout* selected_layout = new FlowLayout;

	std::string current_tile;
	std::unordered_map<std::string, PathingOptions> pathing_options;
};