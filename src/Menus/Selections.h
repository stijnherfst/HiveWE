#pragma once

#include "ui_Selections.h"

class Selections : public QDialog {
	Q_OBJECT

public:
	Selections(QWidget* parent = nullptr);
	~Selections();

private:

	Ui::Selections ui;
};