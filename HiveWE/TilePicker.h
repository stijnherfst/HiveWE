#pragma once

#include "ui_TilePicker.h"

class TilePicker : public QDialog {
	Q_OBJECT

public:
	TilePicker(QWidget* parent, std::vector<std::string> from_ids, std::vector<std::string> to_ids);
	~TilePicker() = default;

signals:
	void tile_chosen(std::string from_id, std::string to_id);

private:
	void accepted();

	Ui::TilePicker ui;

	QButtonGroup* from_group = new QButtonGroup;
	QButtonGroup* to_group = new QButtonGroup;

	FlowLayout* from_layout = new FlowLayout;
	FlowLayout* to_layout = new FlowLayout;
};
