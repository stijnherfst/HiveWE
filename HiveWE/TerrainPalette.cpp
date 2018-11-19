#include "stdafx.h"
#include "TerrainPalette.h"

TerrainPalette::TerrainPalette(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	show();

	brush.tile_id = map->terrain.tileset_ids.front();
	brush.create();
	map->brush = &brush;

	ui.flowLayout_placeholder->addLayout(textures_layout);
	ui.flowLayout_placeholder_2->addLayout(cliff_layout);

	// Ground Tiles
	slk::SLK& slk = map->terrain.terrain_slk;
	for (auto&& i : map->terrain.tileset_ids) {
		const auto image = resource_manager.load<Texture>(slk.data("dir", i) + "/" + slk.data("file", i) + ".blp");
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
	const auto image = resource_manager.load<Texture>("TerrainArt/Blight/Ashen_Blight.blp");
	const auto icon = ground_texture_to_icon(image->data.data(), image->width, image->height);

	ui.blight->setIcon(icon);
	ui.blight->setFixedSize(48, 48);
	ui.blight->setIconSize({ 48, 48 });
	ui.blight->setCheckable(true);
	ui.blight->setProperty("tileID", "blight");
	ui.blight->setProperty("tileName", "Blight");
	textures_group->addButton(ui.blight);

	// Ribbon
	QRibbonSection* settings_section = new QRibbonSection;
	settings_section->setText("Settings");

	QRibbonButton* enforce_water_height_limit = new QRibbonButton;
	enforce_water_height_limit->setText("Enforce Water\nHeight Limit");
	enforce_water_height_limit->setIcon(QIcon("Data/Icons/Ribbon/variation32x32.png"));
	enforce_water_height_limit->setCheckable(true);
	enforce_water_height_limit->setChecked(true);
	settings_section->addWidget(enforce_water_height_limit);

	ribbon_tab->addSection(settings_section);

	connect(enforce_water_height_limit, &QRibbonButton::toggled, [&](bool checked) { brush.enforce_water_height_limits = checked; });

	connect(ui.brushSizeButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		ui.brushSizeSlider->setValue(button->text().toInt());
	});
	connect(ui.brushSizeSlider, &QSlider::valueChanged, [&](int value) {
		brush.set_size(value - 1);
		ui.brushSize->setValue(value);
	});

	connect(ui.textureGroupBox, &QGroupBox::clicked, [&](bool checked) { brush.apply_texture = checked; });
	connect(ui.cliffGroupBox, &QGroupBox::clicked, [&](bool checked) { brush.apply_cliff = checked; });
	connect(ui.deformationGroupBox, &QGroupBox::clicked, [&](bool checked) { brush.apply_height = checked; });

	connect(textures_group, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		brush.tile_id = button->property("tileID").toString().toStdString();
		ui.textureGroupBox->setChecked(true);
		brush.apply_texture = true;
	});

	connect(cliff_group, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		brush.cliff_id = button->property("cliffID").toInt();
		ui.cliffGroupBox->setChecked(true);
		brush.apply_cliff = true;
	});

	connect(ui.cliffButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&]() {
		ui.cliffGroupBox->setChecked(true);
		ui.deformationGroupBox->setChecked(false);
		brush.apply_cliff = true;
		brush.apply_height = false;
	});

	connect(ui.deformationButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&]() {
		ui.deformationGroupBox->setChecked(true);
		ui.cliffGroupBox->setChecked(false);
		brush.apply_height = true;
		brush.apply_cliff = false;
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

	connect(ui.tilePathing, &QCheckBox::clicked, [&](bool checked) { brush.apply_tile_pathing = checked; });
	connect(ui.cliffPathing, &QCheckBox::clicked, [&](bool checked) { brush.apply_cliff_pathing = checked; });
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