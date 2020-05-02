#pragma once

#include "ui_MapInfoEditor.h"

class MapInfoEditor : public QDialog {
	Q_OBJECT

public:
	MapInfoEditor(QWidget* parent = nullptr);

	Ui::MapInfoEditor ui;

	void save() const;
};