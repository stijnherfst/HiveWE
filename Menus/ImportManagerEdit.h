#pragma once

#include "ui_ImportManagerEdit.h"

#include <QDialog>

class ImportManagerEdit : public QDialog {
	Q_OBJECT

public:
	ImportManagerEdit(QWidget* parent = nullptr);

	Ui::ImportManagerEdit ui;

	signals:
	void accepted(bool custom, QString path);
};