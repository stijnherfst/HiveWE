#include "main_ribbon.h"

MainRibbon::MainRibbon(QWidget* parent) : QRibbon(parent) {
	// Home
	QRibbonTab* home_tab = new QRibbonTab;

	// Undo/Redo History
	QRibbonSection* history_section = new QRibbonSection;
	history_section->setText("History");

	undo->setIcon(QIcon("Data/Icons/Ribbon/undo32x32.png"));
	undo->setText("Undo");
	history_section->addWidget(undo);

	redo->setIcon(QIcon("Data/Icons/Ribbon/redo32x32.png"));
	redo->setText("Redo");
	history_section->addWidget(redo);

	//view_history->setIcon(QIcon("Data/Icons/Ribbon/description32x32.png"));
	//view_history->setText("View\nHistory");
	//view_history->setEnabled(false);
	//history_section->addWidget(view_history);

	/*copy->setIcon(QIcon("Data/Icons/Ribbon/copy32x32.ico"));
	copy->setText("Copy");
	home_section->addWidget(copy);

	paste->setIcon(QIcon("Data/Icons/Ribbon/paste32x32.ico"));
	paste->setText("Paste");
	home_section->addWidget(paste);

	QVBoxLayout* lay = new QVBoxLayout;
	QToolButton* but = new QToolButton;
	QToolButton* butt = new QToolButton;
	QToolButton* buttt = new QToolButton;

	but->setIcon(QIcon("Data/Icons/Ribbon/paste32x32.ico"));
	but->setIconSize({ 16, 16 });
	but->setText("Cut");
	but->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);*/

	//lay->addWidget(but);
	//lay->addWidget(butt);
	//lay->addWidget(buttt);

	//history_section->addLayout(lay);
	home_tab->addSection(history_section);

	// View
	QRibbonTab* view_tab = new QRibbonTab;

	// Visible section
	QRibbonSection* visible_section = new QRibbonSection;
	visible_section->setText("Visible");
	view_tab->addSection(visible_section);
	
	units_visible->setIcon(QIcon("Data/Icons/Ribbon/units32x32.png"));
	units_visible->setText("Units");
	units_visible->setCheckable(true);
	units_visible->setChecked(true);
	visible_section->addWidget(units_visible);

	doodads_visible->setIcon(QIcon("Data/Icons/Ribbon/doodads32x32.png"));
	doodads_visible->setText("Doodads");
	doodads_visible->setCheckable(true);
	doodads_visible->setChecked(true);
	visible_section->addWidget(doodads_visible);

	pathing_visible->setIcon(QIcon("Data/Icons/Ribbon/pathing32x32.png"));
	pathing_visible->setText("Pathing");
	pathing_visible->setCheckable(true);
	visible_section->addWidget(pathing_visible);
	
	brush_visible->setIcon(QIcon("Data/Icons/Ribbon/brush32x32.png"));
	brush_visible->setText("Brush");
	brush_visible->setCheckable(true);
	brush_visible->setChecked(true);
	visible_section->addWidget(brush_visible);

	lighting_visible->setIcon(QIcon("Data/Icons/Ribbon/lighting32x32.png"));
	lighting_visible->setText("Lighting");
	lighting_visible->setCheckable(true);
	lighting_visible->setChecked(true);
	visible_section->addWidget(lighting_visible);

	wireframe_visible->setIcon(QIcon("Data/Icons/Ribbon/wireframe32x32.png"));
	wireframe_visible->setText("Wireframe");
	wireframe_visible->setCheckable(true);
	visible_section->addWidget(wireframe_visible);

	debug_visible->setIcon(QIcon("Data/Icons/Ribbon/debug32x32.png"));
	debug_visible->setText("Debug");
	debug_visible->setCheckable(true);
	visible_section->addWidget(debug_visible);

	minimap_visible->setIcon(QIcon("Data/Icons/Ribbon/minimap32x32.png"));
	minimap_visible->setText("Minimap");
	minimap_visible->setCheckable(true);
	minimap_visible->setChecked(true);
	visible_section->addWidget(minimap_visible);
	// Camera section
	QRibbonSection* camera_section = new QRibbonSection;
	camera_section->setText("Camera");
	view_tab->addSection(camera_section);

	reset_camera->setIcon(QIcon("Data/Icons/Ribbon/reset32x32.png"));
	reset_camera->setText("Reset");
	camera_section->addWidget(reset_camera);

	// Menu actions
	new_map->setText("New Map");
	new_map->setIcon(QIcon("Data/Icons/Ribbon/new32x32.ico"));
	new_map->setIconSize({ 32, 32 });
	new_map->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	new_map->setDisabled(true);
	addMenuItem(new_map);

	open_map_folder->setText("Open Map (Folder)");
	open_map_folder->setIcon(QIcon("Data/Icons/Ribbon/open32x32.png"));
	open_map_folder->setIconSize({ 32, 32 });
	open_map_folder->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(open_map_folder);

	open_map_mpq->setText("Open Map (MPQ)");
	open_map_mpq->setIcon(QIcon("Data/Icons/Ribbon/open32x32.png"));
	open_map_mpq->setIconSize({ 32, 32 });
	open_map_mpq->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(open_map_mpq);

	save_map->setText("Save Map");
	save_map->setIcon(QIcon("Data/Icons/Ribbon/save32x32.png"));
	save_map->setIconSize({ 32, 32 });
	save_map->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(save_map);

	save_map_as->setText("Save Map as");
	save_map_as->setIcon(QIcon("Data/Icons/Ribbon/saveas32x32.png"));
	save_map_as->setIconSize({ 32, 32 });
	save_map_as->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(save_map_as);

	export_mpq->setText("Export MPQ");
	export_mpq->setIcon(QIcon("Data/Icons/Ribbon/saveas32x32.png"));
	export_mpq->setIconSize({ 32, 32 });
	export_mpq->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(export_mpq);

	test_map->setText("Test Map");
	test_map->setIcon(QIcon("Data/Icons/Ribbon/test32x32.ico"));
	test_map->setIconSize({ 32, 32 });
	test_map->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(test_map);

	settings->setText("Settings");
	settings->setIcon(QIcon("Data/Icons/Ribbon/options32x32.png"));
	settings->setIconSize({ 32, 32 });
	settings->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(settings);

	addMenuSeperator();

	exit->setText("Exit");
	exit->setIcon(QIcon("Data/Icons/Ribbon/exit32x32.ico"));
	exit->setIconSize({ 32, 32 });
	exit->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(exit);

	// Map tab
	QRibbonTab* map_tab = new QRibbonTab;

	QRibbonSection* map_section = new QRibbonSection;

	map_description->setIcon(QIcon("Data/Icons/Ribbon/description32x32.png"));
	map_description->setText("Description");
	map_section->addWidget(map_description);

	map_loading_screen->setIcon(QIcon("Data/Icons/Ribbon/loading32x32.png"));
	map_loading_screen->setText("Loading\nScreen");
	map_section->addWidget(map_loading_screen);

	map_options->setIcon(QIcon("Data/Icons/Ribbon/options32x32.png"));
	map_options->setText("Options");
	map_section->addWidget(map_options);

	//map_size_camera_bounds->setIcon(QIcon("Data/Icons/Ribbon/sizebounds32x32.png"));
	//map_size_camera_bounds->setText("Size&&Camera\nBounds");
	//map_section->addWidget(map_size_camera_bounds);

	map_tab->addSection(map_section);

	// Tools tab
	QRibbonTab* tools_tab = new QRibbonTab;

	// Import
	QRibbonSection* import_section = new QRibbonSection;
	import_section->setText("Import");

	import_heightmap->setIcon(QIcon("Data/Icons/Ribbon/heightmap32x32.png"));
	import_heightmap->setText("Import\nHeightmap");
	import_section->addWidget(import_heightmap);

	tools_tab->addSection(import_section);

	// Tileset
	QRibbonSection* tileset_section = new QRibbonSection;
	tileset_section->setText("Tileset");

	change_tileset->setIcon(QIcon("Data/Icons/Ribbon/tileset32x32.png"));
	change_tileset->setText("Change\nTileset");
	tileset_section->addWidget(change_tileset);

	change_tile_pathing->setIcon(QIcon("Data/Icons/Ribbon/tileset32x32.png"));
	change_tile_pathing->setText("Change Tile\nPathing");
	tileset_section->addWidget(change_tile_pathing);

	tools_tab->addSection(tileset_section);

	QRibbonSection* game_section = new QRibbonSection;
	game_section->setText("Game");

	switch_warcraft->setIcon(QIcon("Data/Icons/Ribbon/WarIII32x32.ico"));
	switch_warcraft->setText("Change\n Game folder");
	game_section->addWidget(switch_warcraft);

	tools_tab->addSection(game_section);	

	// Window Tab
	QRibbonTab* window_tab = new QRibbonTab;

	QRibbonSection* editor_section = new QRibbonSection;
	editor_section->setText("Editor/Viewer");

	object_editor->setIcon(QIcon("Data/Icons/Ribbon/objecteditor32x32.png"));
	object_editor->setText("Object\nEditor");
	editor_section->addWidget(object_editor);

	model_editor->setIcon(QIcon("Data/Icons/Ribbon/objecteditor32x32.png"));
	model_editor->setText("Model\nEditor");
	model_editor->setEnabled(false);
	editor_section->addWidget(model_editor);

	QRibbonSection* palette_section = new QRibbonSection;
	palette_section->setText("Palette"); 
	
	terrain_palette->setIcon(QIcon("Data/Icons/Ribbon/heightmap32x32.png"));
	terrain_palette->setText("Terrain");
	palette_section->addWidget(terrain_palette);

	doodad_palette->setIcon(QIcon("Data/Icons/Ribbon/doodads32x32.png"));
	doodad_palette->setText("Doodads");
	palette_section->addWidget(doodad_palette);

	unit_palette->setIcon(QIcon("Data/Icons/Ribbon/doodads32x32.png"));
	unit_palette->setText("Units");
	palette_section->addWidget(unit_palette);

	pathing_palette->setIcon(QIcon("Data/Icons/Ribbon/pathing32x32.png"));
	pathing_palette->setText("Pathing");
	palette_section->addWidget(pathing_palette);

	window_tab->addSection(editor_section);
	window_tab->addSection(palette_section);


	addTab(home_tab, "Home");
	addTab(view_tab, "View");
	addTab(map_tab, "Map");
	addTab(tools_tab, "Tools");
	addTab(window_tab, "Window");

}

MainRibbon::~MainRibbon() {
}