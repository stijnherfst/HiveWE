#pragma once

#include <QMainWindow>
#include <QFileDialog>
#include <QSettings>
#include <QObject>
#include <QTimer>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QKeyEvent>

#include "ui_HiveWE.h"
#include "global_search.h"

import QRibbon;
import WindowHandler;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;
import "palette.h";
import "minimap.h";

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

	QElapsedTimer double_shift_timer;

	void keyPressEvent(QKeyEvent* event) override {
		if (event->key() == Qt::Key_Shift && !event->isAutoRepeat()) {
			if (double_shift_timer.isValid() && double_shift_timer.elapsed() < 400) {

				GlobalSearchWidget search_widget = new GlobalSearchWidget(this);
				double_shift_timer.invalidate();
			} else {
				double_shift_timer.start();
			}
		}
		QMainWindow::keyPressEvent(event);
	}

	void closeEvent(QCloseEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void moveEvent(QMoveEvent* event) override;

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
			palette->move(this->x() + this->width() - palette->width() - 10, this->y() + 160);
			connect(palette, &T::ribbon_tab_requested, this, &HiveWE::set_current_custom_tab);
			connect(this, &HiveWE::palette_changed, palette, &Palette::deactivate);
			connect(palette, &T::finished, [&]() {
				remove_custom_tab();
			});
		}
	}

signals:
	void tileset_changed();
	void palette_changed(QRibbonTab* tab);

	void saving_initiated();
};