#pragma once

class MainRibbon : public QRibbon {
	Q_OBJECT

public:
	QRibbonButton* copy = new QRibbonButton;
	QRibbonButton* paste = new QRibbonButton;

	QRibbonButton* units_visible = new QRibbonButton;
	QRibbonButton* doodads_visible = new QRibbonButton;
	QRibbonButton* pathing_visible = new QRibbonButton;
	QRibbonButton* brush_visible = new QRibbonButton;
	QRibbonButton* lighting_visible = new QRibbonButton;
	QRibbonButton* wireframe_visible = new QRibbonButton;
	QRibbonButton* debug_visible = new QRibbonButton;

	QRibbonButton* switch_camera = new QRibbonButton;
	QRibbonButton* reset_camera = new QRibbonButton;

	QRibbonButton* import_heightmap = new QRibbonButton;
	QRibbonButton* change_tileset = new QRibbonButton;
	QRibbonButton* change_tile_pathing = new QRibbonButton;

	QToolButton* new_map = new QToolButton;
	QToolButton* open_map = new QToolButton;
	QToolButton* save_map = new QToolButton;
	QToolButton* save_map_as = new QToolButton;
	QToolButton* test_map = new QToolButton;
	QToolButton* exit = new QToolButton;

	MainRibbon(QWidget* parent);
	~MainRibbon();
};
