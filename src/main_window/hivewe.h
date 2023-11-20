#pragma once


#include <QMainWindow>
#include <QFileDialog>
#include <QSettings>
#include <QObject>
#include <QMessageBox>
#include <QTimer>
#include <QProcess>
#include <QPushButton>
#include <QApplication>
#include <QShortcut>
#include <QToolButton>
#include <QFrame>
#include <QGridLayout>
#include <QBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QTabWidget>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QMap>
#include <QScrollArea>
#include <QPushButton>
#include <QKeySequence>

#include "ui_HiveWE.h"

#include "palette.h"
#include "minimap.h"
#include "globals.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

import QRibbon;
import INI;
import Map;
import SLK;
import WindowHandler;

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
				disconnect(this, &HiveWE::palette_changed, palette, &Palette::deactivate);
			});
		}
	}

signals:
	void tileset_changed();
	void palette_changed(QRibbonTab* tab);

	void saving_initiated();
};