#pragma once

#include <QWidget>
#include "ui_TilePathing.h"

class TilePathing : public QWidget
{
	Q_OBJECT

public:
	TilePathing(QWidget *parent = Q_NULLPTR);
	~TilePathing();

private:
	Ui::TilePathing ui;
};
