#pragma once

#include "ui_Minimap.h"

class Minimap : public QDialog {
	Q_OBJECT

public:
	Minimap(QWidget *parent = Q_NULLPTR);
	~Minimap();

private:
	Ui::Minimap ui;
};
