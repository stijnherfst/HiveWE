#pragma once

#include "ui_HiveWE.h"

class HiveWE : public QMainWindow {
	Q_OBJECT

public:
	explicit HiveWE(QWidget* parent = nullptr);
	void load();
	void save_as();

private:
	Ui::HiveWEClass ui;
	void closeEvent(QCloseEvent* event) override;

signals:
	void tileset_changed();
};

extern Map map;
extern ini::INI world_edit_data;