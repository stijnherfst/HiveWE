#include "main_ribbon.h"

MainRibbon::MainRibbon(QWidget* parent) : QRibbon(parent) {
	// Home
	QRibbonTab* home_tab = new QRibbonTab;

	// Undo/Redo History
	QRibbonSection* history_section = new QRibbonSection;
	history_section->setText("History");

	undo->setIcon(QIcon("data/icons/ribbon/undo.png"));
	undo->setText("Undo");
	history_section->addWidget(undo);

	redo->setIcon(QIcon("data/icons/ribbon/redo.png"));
	redo->setText("Redo");
	history_section->addWidget(redo);

	QRibbonSection* editor_section = new QRibbonSection;
	editor_section->setText("Editor/Viewer");

	trigger_editor->setIcon(QIcon("Data/Icons/Ribbon/triggereditor.png"));
	trigger_editor->setText("Trigger\nEditor");
	editor_section->addWidget(trigger_editor);

	object_editor->setIcon(QIcon("data/icons/ribbon/objecteditor.png"));
	object_editor->setText("Object\nEditor");
	editor_section->addWidget(object_editor);

	model_editor->setIcon(QIcon("data/icons/ribbon/model_editor.png"));
	model_editor->setText("Model\nEditor");
	editor_section->addWidget(model_editor);

	asset_manager->setIcon(QIcon("data/icons/ribbon/asset_manager.png"));
	asset_manager->setText("Asset\nManager");
	editor_section->addWidget(asset_manager);

	QRibbonSection* palette_section = new QRibbonSection;
	palette_section->setText("Palette");

	terrain_palette->setIcon(QIcon("data/icons/ribbon/heightmap.png"));
	terrain_palette->setText("Terrain");
	palette_section->addWidget(terrain_palette);

	doodad_palette->setIcon(QIcon("data/icons/ribbon/doodads.png"));
	doodad_palette->setText("Doodads");
	palette_section->addWidget(doodad_palette);

	unit_palette->setIcon(QIcon("data/icons/ribbon/units.png"));
	unit_palette->setText("Units");
	palette_section->addWidget(unit_palette);

	pathing_palette->setIcon(QIcon("data/icons/ribbon/pathing.png"));
	pathing_palette->setText("Pathing");
	palette_section->addWidget(pathing_palette);

	region_palette->setIcon(QIcon("data/icons/ribbon/sizebounds.png"));
	region_palette->setText("Regions");
	palette_section->addWidget(region_palette);



	//view_history->setIcon(QIcon("data/icons/ribbon/description.png"));
	//view_history->setText("View\nHistory");
	//view_history->setEnabled(false);
	//history_section->addWidget(view_history);

	/*copy->setIcon(QIcon("data/icons/ribbon/copy.ico"));
	copy->setText("Copy");
	home_section->addWidget(copy);

	paste->setIcon(QIcon("data/icons/ribbon/paste.ico"));
	paste->setText("Paste");
	home_section->addWidget(paste);

	QVBoxLayout* lay = new QVBoxLayout;
	QToolButton* but = new QToolButton;
	QToolButton* butt = new QToolButton;
	QToolButton* buttt = new QToolButton;

	but->setIcon(QIcon("data/icons/ribbon/paste.ico"));
	but->setIconSize({ 16, 16 });
	but->setText("Cut");
	but->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);*/

	//lay->addWidget(but);
	//lay->addWidget(butt);
	//lay->addWidget(buttt);

	//history_section->addLayout(lay);
	home_tab->addSection(history_section);
	home_tab->addSection(editor_section);
	home_tab->addSection(palette_section);

	// View
	QRibbonTab* view_tab = new QRibbonTab;

	// Visible section
	QRibbonSection* visible_section = new QRibbonSection;
	visible_section->setText("Visible");
	view_tab->addSection(visible_section);
	
	units_visible->setIcon(QIcon("data/icons/ribbon/units.png"));
	units_visible->setText("Units");
	units_visible->setCheckable(true);
	units_visible->setChecked(true);
	visible_section->addWidget(units_visible);

	doodads_visible->setIcon(QIcon("data/icons/ribbon/doodads.png"));
	doodads_visible->setText("Doodads");
	doodads_visible->setCheckable(true);
	doodads_visible->setChecked(true);
	visible_section->addWidget(doodads_visible);

	pathing_visible->setIcon(QIcon("data/icons/ribbon/pathing.png"));
	pathing_visible->setText("Pathing");
	pathing_visible->setCheckable(true);
	visible_section->addWidget(pathing_visible);

	regions_visible->setIcon(QIcon("data/icons/ribbon/sizebounds.png"));
	regions_visible->setText("Regions");
	regions_visible->setCheckable(true);
	visible_section->addWidget(regions_visible);
	
	brush_visible->setIcon(QIcon("data/icons/ribbon/brush.png"));
	brush_visible->setText("Brush");
	brush_visible->setCheckable(true);
	brush_visible->setChecked(true);
	visible_section->addWidget(brush_visible);

	lighting_visible->setIcon(QIcon("data/icons/ribbon/lighting.png"));
	lighting_visible->setText("Lighting");
	lighting_visible->setCheckable(true);
	lighting_visible->setChecked(true);
	visible_section->addWidget(lighting_visible);

	water_visible->setIcon(QIcon("data/icons/ribbon/water.png"));
	water_visible->setText("Water");
	water_visible->setCheckable(true);
	water_visible->setChecked(true);
	visible_section->addWidget(water_visible);

	click_helpers_visible->setIcon(QIcon("data/icons/ribbon/click_helpers.png"));
	click_helpers_visible->setText("Click\nHelpers");
	click_helpers_visible->setCheckable(true);
	click_helpers_visible->setChecked(true);
	visible_section->addWidget(click_helpers_visible);

	wireframe_visible->setIcon(QIcon("data/icons/ribbon/wireframe.png"));
	wireframe_visible->setText("Wireframe");
	wireframe_visible->setCheckable(true);
	visible_section->addWidget(wireframe_visible);

	debug_visible->setIcon(QIcon("data/icons/ribbon/debug.png"));
	debug_visible->setText("Debug");
	debug_visible->setCheckable(true);
	visible_section->addWidget(debug_visible);

	minimap_visible->setIcon(QIcon("data/icons/ribbon/minimap.png"));
	minimap_visible->setText("Minimap");
	minimap_visible->setCheckable(true);
	minimap_visible->setChecked(true);
	visible_section->addWidget(minimap_visible);
	// Camera section
	QRibbonSection* camera_section = new QRibbonSection;
	camera_section->setText("Camera");
	view_tab->addSection(camera_section);

	reset_camera->setIcon(QIcon("data/icons/ribbon/reset.png"));
	reset_camera->setText("Reset");
	camera_section->addWidget(reset_camera);

	// Menu actions
	new_map->setText("New Map");
	new_map->setIcon(QIcon("data/icons/ribbon/new.ico"));
	new_map->setIconSize({ 32, 32 });
	new_map->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(new_map);

	open_map_folder->setText("Open Map (Folder)");
	open_map_folder->setIcon(QIcon("data/icons/ribbon/open.png"));
	open_map_folder->setIconSize({ 32, 32 });
	open_map_folder->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(open_map_folder);

	open_map_mpq->setText("Open Map (MPQ)");
	open_map_mpq->setIcon(QIcon("data/icons/ribbon/open.png"));
	open_map_mpq->setIconSize({ 32, 32 });
	open_map_mpq->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(open_map_mpq);

	save_map->setText("Save Map");
	save_map->setIcon(QIcon("data/icons/ribbon/save.png"));
	save_map->setIconSize({ 32, 32 });
	save_map->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(save_map);

	save_map_as->setText("Save Map as");
	save_map_as->setIcon(QIcon("data/icons/ribbon/saveas.png"));
	save_map_as->setIconSize({ 32, 32 });
	save_map_as->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(save_map_as);

	export_map->setText("Export Map");
	export_map->setIcon(QIcon("data/icons/ribbon/saveas.png"));
	export_map->setIconSize({ 32, 32 });
	export_map->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(export_map);

	test_map->setText("Test Map");
	test_map->setIcon(QIcon("data/icons/ribbon/test.ico"));
	test_map->setIconSize({ 32, 32 });
	test_map->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(test_map);

	settings->setText("Settings");
	settings->setIcon(QIcon("data/icons/ribbon/options.png"));
	settings->setIconSize({ 32, 32 });
	settings->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(settings);

	addMenuSeperator();

	exit->setText("Exit");
	exit->setIcon(QIcon("data/icons/ribbon/exit.ico"));
	exit->setIconSize({ 32, 32 });
	exit->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addMenuItem(exit);

	// Map tab
	QRibbonTab* map_tab = new QRibbonTab;

	QRibbonSection* map_section = new QRibbonSection;

	map_description->setIcon(QIcon("data/icons/ribbon/description.png"));
	map_description->setText("Description");
	map_section->addWidget(map_description);

	map_loading_screen->setIcon(QIcon("data/icons/ribbon/loading.png"));
	map_loading_screen->setText("Loading\nScreen");
	map_section->addWidget(map_loading_screen);

	map_options->setIcon(QIcon("data/icons/ribbon/options.png"));
	map_options->setText("Options");
	map_section->addWidget(map_options);

	gameplay_constants->setIcon(QIcon("data/icons/ribbon/options.png"));
	gameplay_constants->setText("Gameplay\nConstants");
	map_section->addWidget(gameplay_constants);

	map_protection->setIcon(QIcon("data/icons/ribbon/options.png"));
	map_protection->setText("Protection");
	map_section->addWidget(map_protection);

	//map_size_camera_bounds->setIcon(QIcon("data/icons/ribbon/sizebounds.png"));
	//map_size_camera_bounds->setText("Size&&Camera\nBounds");
	//map_section->addWidget(map_size_camera_bounds);

	map_tab->addSection(map_section);

	// Tools tab
	QRibbonTab* tools_tab = new QRibbonTab;

	// Import
	QRibbonSection* import_section = new QRibbonSection;
	import_section->setText("Import");

	import_heightmap->setIcon(QIcon("data/icons/ribbon/heightmap.png"));
	import_heightmap->setText("Import\nHeightmap");
	import_section->addWidget(import_heightmap);

	tools_tab->addSection(import_section);

	// Tileset
	QRibbonSection* tileset_section = new QRibbonSection;
	tileset_section->setText("Tileset");

	change_tileset->setIcon(QIcon("data/icons/ribbon/tileset.png"));
	change_tileset->setText("Change\nTileset");
	tileset_section->addWidget(change_tileset);

	change_tile_pathing->setIcon(QIcon("data/icons/ribbon/tileset.png"));
	change_tile_pathing->setText("Change Tile\nPathing");
	tileset_section->addWidget(change_tile_pathing);

	tools_tab->addSection(tileset_section);

	QRibbonSection* game_section = new QRibbonSection;
	game_section->setText("Game");

	switch_warcraft->setIcon(QIcon("data/icons/ribbon/WarIII.ico"));
	switch_warcraft->setText("Change\n Game folder");
	game_section->addWidget(switch_warcraft);

	tools_tab->addSection(game_section);

	addTab(home_tab, "Home");
	addTab(view_tab, "View");
	addTab(map_tab, "Map");
	addTab(tools_tab, "Tools");

}

MainRibbon::~MainRibbon() {
}