#pragma once

#include "ui_DoodadPalette.h"

class DoodadPalette : public QDialog
{
	Q_OBJECT

public:
	DoodadPalette(QWidget* parent = nullptr);
	~DoodadPalette();

private:
	Ui::DoodadPalette ui;
};