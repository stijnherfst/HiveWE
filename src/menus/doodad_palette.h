#pragma once

#include "ui_doodad_palette.h"

#include <QLineEdit>
#include <QRadioButton>
#include <QCheckBox>
#include <QFormLayout>
#include <QDoubleValidator>
#include <QComboBox>
#include <QListView>
#include <QToolButton>
#include <QShortcut>
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
#include <QTimer>

#include "doodad_brush.h"
#include "palette.h"

#include <QConcatenateTablesProxyModel>

#include <string>

import QRibbon;
import AspectRatioPixmapLabel;
import DestructibleListModel;
import DoodadListModel;

///	Only allows inputting doubles and automatically selects the text on focus
class DoubleInput : public QLineEdit {
	void focusInEvent(QFocusEvent* event) override {
		QLineEdit::focusInEvent(event);
		QTimer::singleShot(0, this, &QLineEdit::selectAll);
	}

public:
	DoubleInput() {
		setValidator(new QDoubleValidator(this));
	}
};

class DoodadPalette : public Palette {
	Q_OBJECT

public:
	DoodadPalette(QWidget* parent = nullptr);
	~DoodadPalette();

private:
	bool event(QEvent *e) override;

	void selection_changed(const QModelIndex& index);
	void select_id_in_palette(std::string id);

	Ui::DoodadPalette ui;

	DoodadBrush brush;
	DoodadListModel* doodad_list_model;
	DoodadListFilter* doodad_filter_model;
	DestructableListModel* destructable_list_model;
	DestructableListFilter* destructable_filter_model;
	QConcatenateTablesProxyModel* concat_table;

	QRibbonTab* ribbon_tab = new QRibbonTab;
	QRibbonButton* selection_mode = new QRibbonButton;
	QRibbonButton* selections_button = new QRibbonButton;

	QRibbonContainer* variations = new QRibbonContainer;

	QRibbonSection* current_selection_section = new QRibbonSection;
	DoubleInput* x_scale = new DoubleInput;
	DoubleInput* y_scale = new DoubleInput;
	DoubleInput* z_scale = new DoubleInput;
	DoubleInput* rotation = new DoubleInput;

	DoubleInput* absolute_height = new DoubleInput;
	DoubleInput* relative_height = new DoubleInput;

	QAction* group_height_minimum = new QAction("Minimum");
	QAction* group_height_average = new QAction("Average");
	QAction* group_height_maximum = new QAction("Maximum");

	QLabel* selection_name = new QLabel;

	QShortcut* find_this;
	QShortcut* find_parent;
	QShortcut* change_mode_this;
	QShortcut* change_mode_parent;


public slots:
	void deactivate(QRibbonTab* tab) override;
	void update_selection_info();
	void update_scale_change(int component, const QString& text);
	void update_scale_finish(int component);
	void update_rotation_change(const QString& text);
	void update_absolute_change(const QString& text);
	void update_relative_change(const QString& text);
	void set_group_height_minimum();
	void set_group_height_average();
	void set_group_height_maximum();
	void set_selection_rotation(float rotation);
};