#pragma once

#include "ui_TileSetter.h"

class TileSetter : public QDialog {
	Q_OBJECT

public:
	explicit TileSetter(QWidget *parent = nullptr);
	~TileSetter();

private:
	void add_tile() const;
	void remove_tile() const;
	void update_available_tiles() const;
	void to_tile_clicked(QAbstractButton* button) const;
	void shift_left() const;
	void shift_right() const;
	void save_tiles();

	Ui::TileSetter ui;

	QButtonGroup* selected_group = new QButtonGroup;
	QButtonGroup* available_group = new QButtonGroup;

	FlowLayout* selected_layout = new FlowLayout;
	FlowLayout* available_layout = new FlowLayout;

	std::vector<int> from_to_id;
};