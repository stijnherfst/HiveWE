#pragma once

#include "ui_DoodadPalette.h"

class DoodadPalette : public QDialog {
	Q_OBJECT

public:
	DoodadPalette(QWidget* parent = nullptr);
	~DoodadPalette();

private:
	bool event(QEvent *e) override;

	void set_doodad(QListWidgetItem* item);

	Ui::DoodadPalette ui;
	DoodadBrush brush;
};