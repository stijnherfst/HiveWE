#include "terrain_palette.h"

//#include "Globals.h"
#include <map_global.h>

import OpenGLUtilities;

TerrainPalette::TerrainPalette(QWidget *parent) : Palette(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	show();

	brush.tile_id = map->terrain.tileset_ids.front();
	map->brush = &brush;

	change_mode_this = new QShortcut(Qt::Key_Space, this, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);
	change_mode_parent = new QShortcut(Qt::Key_Space, parent, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);

	ui.flowLayout_placeholder->addLayout(textures_layout);
	ui.flowLayout_placeholder_2->addLayout(cliff_layout);

	// Ground Tiles
	slk::SLK& slk = map->terrain.terrain_slk;
	for (auto&& i : map->terrain.tileset_ids) {
		const auto image = resource_manager.load<Texture>(slk.data("dir", i) + "/" + slk.data("file", i));
		const auto icon = ground_texture_to_icon(image->data.data(), image->width, image->height);

		QPushButton* button = new QPushButton;
		button->setIcon(icon);
		button->setFixedSize(48, 48);
		button->setIconSize({ 48, 48 });
		button->setCheckable(true);
		button->setProperty("tileID", QString::fromStdString(i));
		button->setProperty("tileName", QString::fromStdString(slk.data("comment", i)));

		textures_layout->addWidget(button);
		textures_group->addButton(button);

		auto& cliff_tiles = map->terrain.cliff_to_ground_texture;
		const auto is_cliff_tile = std::find(cliff_tiles.begin(), cliff_tiles.end(), map->terrain.ground_texture_to_id[i]);

		if (is_cliff_tile != cliff_tiles.end()) {
			const int index = std::distance(cliff_tiles.begin(), is_cliff_tile);

			button = new QPushButton;
			button->setIcon(icon);
			button->setFixedSize(48, 48);
			button->setIconSize({ 48, 48 });
			button->setCheckable(true); 
			button->setProperty("cliffID", QString::number(index));
			button->setProperty("tileName", QString::fromStdString(slk.data("comment", i)));

			cliff_layout->addWidget(button);
			cliff_group->addButton(button);
		}
	}

	// Blight texture
	const auto image = resource_manager.load<Texture>("TerrainArt/Blight/Ashen_Blight.dds");
	const auto icon = ground_texture_to_icon(image->data.data(), image->width, image->height);

	ui.blight->setIcon(icon);
	ui.blight->setFixedSize(48, 48);
	ui.blight->setIconSize({ 48, 48 });
	ui.blight->setCheckable(true);
	ui.blight->setProperty("tileID", "blight");
	ui.blight->setProperty("tileName", "Blight");
	textures_group->addButton(ui.blight);

	// Ribbon
	QRibbonSection* selection_section = new QRibbonSection;
	selection_section->setText("Selection");

	selection_mode->setText("Selection\nMode");
	selection_mode->setIcon(QIcon("data/icons/ribbon/select32x32.png"));
	selection_mode->setCheckable(true);
	selection_section->addWidget(selection_mode);

	QRibbonSection* general_section = new QRibbonSection;
	general_section->setText("General");

	QRibbonButton* enforce_water_height_limit = new QRibbonButton;
	enforce_water_height_limit->setText("Enforce Water\nHeight Limit");
	enforce_water_height_limit->setIcon(QIcon("data/icons/ribbon/variation32x32.png"));
	enforce_water_height_limit->setCheckable(true);
	enforce_water_height_limit->setChecked(true);
	general_section->addWidget(enforce_water_height_limit);

	QRibbonButton* change_doodad_heights = new QRibbonButton;
	change_doodad_heights->setText("Update\nDoodad Z");
	change_doodad_heights->setIcon(QIcon("data/icons/ribbon/changeheight32x32.png"));
	change_doodad_heights->setCheckable(true);
	change_doodad_heights->setChecked(true);
	general_section->addWidget(change_doodad_heights);

	QRibbonSection* cliff_section = new QRibbonSection;
	cliff_section->setText("Cliff");

	QRibbonButton* relative_cliff_heights = new QRibbonButton;
	relative_cliff_heights->setText("Relative\nHeight");
	relative_cliff_heights->setIcon(QIcon("data/icons/ribbon/changeheight32x32.png"));
	relative_cliff_heights->setCheckable(true);
	relative_cliff_heights->setChecked(false);
	relative_cliff_heights->setEnabled(false);
	cliff_section->addWidget(relative_cliff_heights);

	QRibbonSection* pathing_section = new QRibbonSection;
	pathing_section->setText("Pathing");

	QRibbonButton* apply_cliff_pathing = new QRibbonButton;
	apply_cliff_pathing->setText("Cliff\nPathing");
	apply_cliff_pathing->setIcon(QIcon("data/icons/ribbon/rock32x32.png"));
	apply_cliff_pathing->setCheckable(true);
	apply_cliff_pathing->setChecked(true);
	pathing_section->addWidget(apply_cliff_pathing);

	QRibbonButton* apply_tile_pathing = new QRibbonButton;
	apply_tile_pathing->setText("Tile\nPathing");
	apply_tile_pathing->setIcon(QIcon("data/icons/ribbon/tileset32x32.png"));
	apply_tile_pathing->setCheckable(true);
	apply_tile_pathing->setChecked(true);
	pathing_section->addWidget(apply_tile_pathing);

	QRibbonButton* apply_water_pathing = new QRibbonButton;
	apply_water_pathing->setText("Water\nPathing");
	apply_water_pathing->setIcon(QIcon("data/icons/ribbon/water32x32.png"));
	apply_water_pathing->setCheckable(true);
	apply_water_pathing->setChecked(true);
	pathing_section->addWidget(apply_water_pathing);

	ribbon_tab->addSection(selection_section);
	ribbon_tab->addSection(general_section);
	ribbon_tab->addSection(cliff_section);
	ribbon_tab->addSection(pathing_section);

	connect(selection_mode, &QRibbonButton::toggled, [&]() { brush.switch_mode(); });
	connect(change_mode_this, &QShortcut::activated, [&]() {
		selection_mode->click();
	});

	connect(change_mode_parent, &QShortcut::activated, [&]() {
		selection_mode->click();
	});

	connect(enforce_water_height_limit, &QRibbonButton::toggled, [&](bool checked) { brush.enforce_water_height_limits = checked; });
	connect(change_doodad_heights, &QRibbonButton::toggled, [&](bool checked) { brush.change_doodad_heights = checked; });
	connect(relative_cliff_heights, &QRibbonButton::toggled, [&](bool checked) { brush.relative_cliff_heights = checked; });

	connect(enforce_water_height_limit, &QRibbonButton::toggled, [&](bool checked) { brush.enforce_water_height_limits = checked; });

	connect(apply_cliff_pathing, &QRibbonButton::toggled, [&](bool checked) { brush.apply_cliff_pathing = checked; });
	connect(apply_tile_pathing, &QRibbonButton::toggled, [&](bool checked) { brush.apply_tile_pathing = checked; });
	connect(apply_water_pathing, &QRibbonButton::toggled, [&](bool checked) { brush.apply_water_pathing	= checked; });

	connect(ui.brushSizeButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		ui.brushSizeSlider->setValue(button->text().toInt());
	});
	connect(ui.brushSizeSlider, &QSlider::valueChanged, [&](int value) {
		brush.set_size(value);
		ui.brushSize->setValue(value);
	});
	
	connect(ui.textureCheckbox, &QCheckBox::clicked, [&](bool checked) { brush.apply_texture = checked; });
	connect(ui.cliffCheckbox, &QCheckBox::clicked, [&](bool checked) { brush.apply_cliff = checked; });
	connect(ui.deformationCheckbox, &QCheckBox::clicked, [&](bool checked) { brush.apply_height = checked; });

	connect(textures_group, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		brush.tile_id = button->property("tileID").toString().toStdString();
		ui.deformationCheckbox->setChecked(false);
		ui.cliffCheckbox->setChecked(false);
		ui.textureCheckbox->setChecked(true);
		brush.apply_cliff = false;
		brush.apply_height = false;
		brush.apply_texture = true;
	});

	connect(cliff_group, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		brush.cliff_id = button->property("cliffID").toInt();
		ui.deformationCheckbox->setChecked(false);
		ui.cliffCheckbox->setChecked(true);
		ui.textureCheckbox->setChecked(false);
		brush.apply_cliff = true;
		brush.apply_height = false;
		brush.apply_texture = false;
	});

	connect(ui.cliffButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&]() {
		ui.deformationCheckbox->setChecked(false);
		ui.cliffCheckbox->setChecked(true);
		ui.textureCheckbox->setChecked(false);
		brush.apply_cliff = true;
		brush.apply_height = false;
		brush.apply_texture = false;
	});

	connect(cliff_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), [&]() {
		ui.deformationCheckbox->setChecked(false);
		ui.cliffCheckbox->setChecked(true);
		ui.textureCheckbox->setChecked(false);
		brush.apply_cliff = true;
		brush.apply_height = false;
		brush.apply_texture = false;
	});

	connect(ui.deformationButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&]() {
		ui.deformationCheckbox->setChecked(true);
		ui.cliffCheckbox->setChecked(false);
		ui.textureCheckbox->setChecked(false);
		brush.apply_cliff = false;
		brush.apply_height = true;
		brush.apply_texture = false;
	});

	connect(ui.terrainRaise, &QPushButton::clicked, [&]() { brush.deformation_type = TerrainBrush::deformation::raise; });
	connect(ui.terrainLower, &QPushButton::clicked, [&]() { brush.deformation_type = TerrainBrush::deformation::lower; });
	connect(ui.terrainPlateau, &QPushButton::clicked, [&]() { brush.deformation_type = TerrainBrush::deformation::plateau; });
	connect(ui.terrainRipple, &QPushButton::clicked, [&]() { brush.deformation_type = TerrainBrush::deformation::ripple; });
	connect(ui.terrainSmooth, &QPushButton::clicked, [&]() { brush.deformation_type = TerrainBrush::deformation::smooth; });

	connect(ui.cliffLower2, &QPushButton::clicked, [&]() { brush.cliff_operation_type = TerrainBrush::cliff_operation::lower2; });
	connect(ui.cliffLower1, &QPushButton::clicked, [&]() { brush.cliff_operation_type = TerrainBrush::cliff_operation::lower1; });
	connect(ui.cliffLevel, &QPushButton::clicked, [&]() { brush.cliff_operation_type = TerrainBrush::cliff_operation::level; });
	connect(ui.cliffRaise1, &QPushButton::clicked, [&]() { brush.cliff_operation_type = TerrainBrush::cliff_operation::raise1; });
	connect(ui.cliffRaise2, &QPushButton::clicked, [&]() { brush.cliff_operation_type = TerrainBrush::cliff_operation::raise2; });
	connect(ui.cliffDeepWater, &QPushButton::clicked, [&]() { brush.cliff_operation_type = TerrainBrush::cliff_operation::deep_water; });
	connect(ui.cliffShallowWater, &QPushButton::clicked, [&]() { brush.cliff_operation_type = TerrainBrush::cliff_operation::shallow_water; });
	connect(ui.cliffRamp, &QPushButton::clicked, [&]() { brush.cliff_operation_type = TerrainBrush::cliff_operation::ramp; });

	connect(ui.brushShapeCircle, &QPushButton::clicked, [&]() { brush.set_shape(Brush::Shape::circle); });
	connect(ui.brushShapeSquare, &QPushButton::clicked, [&]() { brush.set_shape(Brush::Shape::square); });
	connect(ui.brushShapeDiamond, &QPushButton::clicked, [&]() { brush.set_shape(Brush::Shape::diamond); });
}

TerrainPalette::~TerrainPalette() {
	map->brush = nullptr;
}

bool TerrainPalette::event(QEvent *e) {
	if (e->type() == QEvent::WindowActivate) {
		map->brush = &brush;
		emit ribbon_tab_requested(ribbon_tab, "Terrain Palette");
	}
	return QWidget::event(e);
}

void TerrainPalette::deactivate(QRibbonTab* tab) {
	if (tab != ribbon_tab) {
		brush.clear_selection();
	}
}