#pragma once

#include "ui_TilePathing.h"

struct PathingOptions {
	bool walkable = true;
	bool flyable = true;
	bool buildable = true;

	enum class Operation {
		replace,
		add,
		remove
	};
	Operation operation = Operation::replace;
};

class TilePathing : public QDialog {
	Q_OBJECT

public:
	explicit TilePathing(QWidget *parent = Q_NULLPTR);
	~TilePathing();

private:
	void changed_tile(QAbstractButton* button);
	void save_tiles();

	Ui::TilePathing ui;
	QButtonGroup* selected_group = new QButtonGroup;
	FlowLayout* selected_layout = new FlowLayout;

	std::string current_tile;
	std::map<std::string, PathingOptions> pathing_options;
};
