#pragma once

#include "ui_DoodadPalette.h"

#include <QLineEdit>

#include "DoodadBrush.h"
#include "Palette.h"
#include "QRibbon.h"
#include "AspectRatioPixmapLabel.h"
#include "DoodadListModel.h"
#include "DestructableListModel.h"

#include <QConcatenateTablesProxyModel>

class DoodadPalette : public Palette {
	Q_OBJECT

public:
	DoodadPalette(QWidget* parent = nullptr);
	~DoodadPalette();

private:
	bool event(QEvent *e) override;

	void selection_changed(const QModelIndex& index);

	Ui::DoodadPalette ui;

	DoodadBrush brush;
	DoodadListModel* doodad_list_model;
	DoodadListFilter* doodad_filter_model;
	DestructableListModel* destructable_list_model;
	DestructableListFilter* destructable_filter_model;
	QConcatenateTablesProxyModel* table;

	QRibbonTab* ribbon_tab = new QRibbonTab;
	QRibbonButton* selection_mode = new QRibbonButton;
	QRibbonButton* selections_button = new QRibbonButton;

	QRibbonContainer* variations = new QRibbonContainer;

	QRibbonSection* pathing_section = new QRibbonSection;
	AspectRatioPixmapLabel* pathing_image_label = new AspectRatioPixmapLabel;

	QRibbonSection* current_selection_section = new QRibbonSection;
	QLineEdit* x_scale = new QLineEdit;
	QLineEdit* y_scale = new QLineEdit;
	QLineEdit* z_scale = new QLineEdit;
	QLineEdit* rotation = new QLineEdit;

	QLineEdit* absolute_height = new QLineEdit;
	QLineEdit* relative_height = new QLineEdit;

	QShortcut* find_this;
	QShortcut* find_parent;


public slots:
	void deactivate(QRibbonTab* tab) override;
	void update_selection_info();
	void update_scale_change(int component, const QString& text);
	void update_scale_finish(int component);
	void update_rotation_change(const QString& text);
	void set_selection_rotation(float rotation);
};