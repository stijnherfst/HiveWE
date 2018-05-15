#include "stdafx.h"
#include "TerrainPalette.h"

TerrainPalette::TerrainPalette(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	show();

	brush.granularity = 4;
	brush.uv_offset_locked = true;
	brush.uv_offset = { 2, 2 };
	brush.brush_offset = { 0.5f, 0.5f };
	brush.tile_id = map.terrain.tileset_ids.front();
	brush.create();
	map.brush = &brush;

	ui.flowLayout_placeholder->addLayout(textures_layout);
	ui.flowLayout_placeholder_2->addLayout(cliff_layout);

	// Ground Tiles
	slk::SLK& slk = map.terrain.terrain_slk;
	for (auto&& i : map.terrain.tileset_ids) {
		const auto image = resource_manager.load<Texture>(slk.data("dir", i) + "/" + slk.data("file", i) + ".blp");
		const auto icon = texture_to_icon(image->data, image->width, image->height);

		QPushButton* button = new QPushButton;
		button->setIcon(icon);
		button->setFixedSize(48, 48);
		button->setIconSize({ 48, 48 });
		button->setCheckable(true);
		button->setProperty("tileID", QString::fromStdString(i));
		button->setProperty("tileName", QString::fromStdString(slk.data("comment", i)));

		textures_layout->addWidget(button);
		textures_group->addButton(button);

		auto& cliff_tiles = map.terrain.cliff_to_ground_texture;
		const auto is_cliff_tile = std::find(cliff_tiles.begin(), cliff_tiles.end(), map.terrain.ground_texture_to_id[i]);

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
	const auto icon = texture_to_icon(image->data, image->width, image->height);

	ui.blight->setIcon(icon);
	ui.blight->setFixedSize(48, 48);
	ui.blight->setIconSize({ 48, 48 });
	ui.blight->setCheckable(true);
	ui.blight->setProperty("tileID", "blight");
	ui.blight->setProperty("tileName", "Blight");
	textures_group->addButton(ui.blight);

	connect(textures_group, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		brush.tile_id = button->property("tileID").toString().toStdString();
		brush.apply_texture = true;
		ui.textureGroupBox->setChecked(true);
	});

	connect(cliff_group, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		brush.cliff_id = button->property("cliffID").toInt();
		brush.apply_cliff = true;
		ui.cliffGroupBox->setChecked(true);
	});

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

	connect(ui.cliffButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&]() {
		brush.apply_cliff = true;
		ui.cliffGroupBox->setChecked(true);
		ui.deformationGroupBox->setChecked(false);
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
	map.brush = nullptr;
}

bool TerrainPalette::event(QEvent *e) {
	if (e->type() == QEvent::WindowActivate) {
		map.brush = &brush;
	}
	return QWidget::event(e);
}