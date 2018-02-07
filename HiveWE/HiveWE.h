#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_HiveWE.h"

class HiveWE : public QMainWindow
{
	Q_OBJECT

public:
	HiveWE(QWidget *parent = Q_NULLPTR);

	void load();
	void save_as();

private:
	Ui::HiveWEClass ui;
};

extern Map map;
