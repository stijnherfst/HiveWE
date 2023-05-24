#pragma once

#include "ui_map_info_editor.h"

class MapInfoEditor : public QDialog {
	Q_OBJECT

public:
	MapInfoEditor(QWidget* parent = nullptr);

	Ui::MapInfoEditor ui;

	void save() const;
};