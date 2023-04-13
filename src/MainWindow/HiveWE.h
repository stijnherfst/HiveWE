#pragma once

#include "ui_HiveWE.h"

#include <QMainWindow>
#include "Palette.h"
#include "Minimap.h"
#include "Map.h"
#include "TableModel.h"
#include "SLK.h"
#include "GLWidget.h"
#include "Globals.h"

import QRibbon;
import INI;

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
		bool created = false;
		auto palette = window_handler.create_or_raise<T>(this, created);
		if (created) {
			palette->move(width() - palette->width() - 10, ui.widget->y() + 29);
			connect(palette, &T::ribbon_tab_requested, this, &HiveWE::set_current_custom_tab);
			connect(this, &HiveWE::palette_changed, palette, &Palette::deactivate);
			connect(palette, &T::finished, [&]() {
				remove_custom_tab();
				disconnect(this, &HiveWE::palette_changed, palette, &Palette::deactivate);
			});
		}
	}

signals:
	void tileset_changed();
	void palette_changed(QRibbonTab* tab);

	void saving_initiated();
};