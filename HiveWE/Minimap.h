#pragma once

#include "ui_Minimap.h"

class Minimap : public QWidget {
	Q_OBJECT

public:
	Minimap(QWidget *parent = Q_NULLPTR);
	~Minimap();

public slots:
	void set_minimap(Texture texture);

private:
	Ui::Minimap ui;
};
