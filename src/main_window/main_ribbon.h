#pragma once

import QRibbon;

#include <QObject>

class MainRibbon : public QRibbon {
	Q_OBJECT

public:
	QRibbonButton* undo = new QRibbonButton;
	QRibbonButton* redo = new QRibbonButton;
	QRibbonButton* view_history = new QRibbonButton;


	QRibbonButton* copy = new QRibbonButton;
	QRibbonButton* paste = new QRibbonButton;

	QRibbonButton* units_visible = new QRibbonButton;
	QRibbonButton* doodads_visible = new QRibbonButton;
	QRibbonButton* pathing_visible = new QRibbonButton;
	QRibbonButton* brush_visible = new QRibbonButton;
	QRibbonButton* lighting_visible = new QRibbonButton;
	QRibbonButton* wireframe_visible = new QRibbonButton;
	QRibbonButton* debug_visible = new QRibbonButton;
	QRibbonButton* minimap_visible = new QRibbonButton;

	QRibbonButton* reset_camera = new QRibbonButton;

	QRibbonButton* map_description = new QRibbonButton;
	QRibbonButton* map_loading_screen = new QRibbonButton;
	QRibbonButton* map_options = new QRibbonButton;
	QRibbonButton* map_size_camera_bounds = new QRibbonButton;

	QRibbonButton* import_heightmap = new QRibbonButton;
	QRibbonButton* change_tileset = new QRibbonButton;
	QRibbonButton* change_tile_pathing = new QRibbonButton;
	QRibbonButton* switch_warcraft = new QRibbonButton;

	QRibbonButton* object_editor = new QRibbonButton;
	QRibbonButton* model_editor = new QRibbonButton;

	QRibbonButton* terrain_palette = new QRibbonButton;
	QRibbonButton* doodad_palette = new QRibbonButton;
	QRibbonButton* unit_palette = new QRibbonButton;
	QRibbonButton* pathing_palette = new QRibbonButton;

	QToolButton* new_map = new QToolButton;
	QToolButton* open_map_mpq = new QToolButton;
	QToolButton* open_map_folder = new QToolButton;

	QToolButton* save_map = new QToolButton;
	QToolButton* save_map_as = new QToolButton;
	QToolButton* export_mpq = new QToolButton;

	QToolButton* test_map = new QToolButton;
	QToolButton* settings = new QToolButton;
	QToolButton* exit = new QToolButton;

	MainRibbon(QWidget* parent);
	~MainRibbon();
};
