#pragma once

#include "ui_DoodadPalette.h"

class DoodadPalette : public QDialog {
	Q_OBJECT

public:
	DoodadPalette(QWidget* parent = nullptr);
	~DoodadPalette();

private:
	bool event(QEvent *e) override;

	void update_list();
	void selection_changed();

	Ui::DoodadPalette ui;
	DoodadBrush brush;
};