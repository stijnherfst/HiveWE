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
	});

	connect(ui.brushSizeButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		brush.set_size(button->text().toInt() - 1);
		ui.brushSize->setValue(button->text().toInt());
	});
	connect(ui.brushSizeSlider, &QSlider::valueChanged, [&](int value) {
		brush.set_size(value - 1);
		ui.brushSize->setValue(value);
	});
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