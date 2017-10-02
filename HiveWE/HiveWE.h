#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_HiveWE.h"

class HiveWE : public QMainWindow
{
	Q_OBJECT

public:
	HiveWE(QWidget *parent = Q_NULLPTR);

private:
	Ui::HiveWEClass ui;
};
