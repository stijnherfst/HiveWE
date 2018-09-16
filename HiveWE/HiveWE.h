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
	QRibbonTab* current_custom_tab = nullptr;


	void closeEvent(QCloseEvent* event) override;

	void switch_camera();
	void switch_warcraft();
	void import_heightmap();

	/// Adds the tab to the ribbon and sets the current index to this tab
	void set_current_custom_tab(QRibbonTab* tab);
	void remove_custom_tab();
signals:
	void tileset_changed();
};

extern Map map;
extern ini::INI world_edit_strings;
extern ini::INI world_edit_game_strings;
extern ini::INI world_edit_data;
extern WindowHandler window_handler;