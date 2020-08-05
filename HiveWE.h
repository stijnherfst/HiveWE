#pragma once

#include "ui_HiveWE.h"

#include <QMainWindow>
#include "WindowHandler.h"
#include "QRibbon.h"
#include "Minimap.h"
#include "Map.h"
#include "INI.h"
#include "TableModel.h"
#include "SLK2.h"

class HiveWE : public QMainWindow {
	Q_OBJECT

public:
	explicit HiveWE(QWidget* parent = nullptr);

	void load_folder();
	void load_mpq();
	void save();
	void save_as();
	void export_mpq();
	void play_test();

private:
	Ui::HiveWEClass ui;
	QRibbonTab* current_custom_tab = nullptr;
	Minimap* minimap = new Minimap(this);

	void closeEvent(QCloseEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void moveEvent(QMoveEvent* event) override;

	void switch_camera();
	void switch_warcraft();
	void import_heightmap();
	void save_window_state();
	void restore_window_state();

	/// Adds the tab to the ribbon and sets the current index to this tab
	void set_current_custom_tab(QRibbonTab* tab, QString name);
	void remove_custom_tab();

	template <typename T>
	void open_palette() {
		auto palette = new T(this);
		palette->move(width() - palette->width() - 10, ui.widget->y() + 29);
		connect(palette, &T::ribbon_tab_requested, this, &HiveWE::set_current_custom_tab);
		connect(this, &HiveWE::palette_changed, palette, &Palette::deactivate);
		connect(palette, &T::finished, [&]() {
			remove_custom_tab();
			disconnect(this, &HiveWE::palette_changed, palette, &Palette::deactivate);
		});
	}

signals:
	void tileset_changed();
	void palette_changed(QRibbonTab* tab);

	void saving_initiated();
};

extern Map* map;
extern ini::INI world_edit_strings;
extern ini::INI world_edit_game_strings;
extern ini::INI world_edit_data;
extern WindowHandler window_handler;

extern TableModel* units_table;
extern slk::SLK2 units_slk;
extern slk::SLK2 units_meta_slk;
extern ini::INI unit_editor_data;

extern TableModel* items_table;
extern slk::SLK2 items_slk;
extern slk::SLK2 items_meta_slk;

extern TableModel* abilities_table;
extern slk::SLK2 abilities_slk;
extern slk::SLK2 abilities_meta_slk;

extern TableModel* doodads_table;
extern slk::SLK2 doodads_slk;
extern slk::SLK2 doodads_meta_slk;

extern TableModel* destructibles_table;
extern slk::SLK2 destructibles_slk;
extern slk::SLK2 destructibles_meta_slk;

extern TableModel* upgrade_table;
extern slk::SLK2 upgrade_slk;
extern slk::SLK2 upgrade_meta_slk;

extern TableModel* buff_table;
extern slk::SLK2 buff_slk;
extern slk::SLK2 buff_meta_slk;

