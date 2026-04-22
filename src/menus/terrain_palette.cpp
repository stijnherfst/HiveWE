#include "terrain_palette.h"
#include "terrain_operators.h"

import MapGlobal;

import std;
import SLK;
import Texture;
import OpenGLUtilities;
import ResourceManager;

TerrainPalette::TerrainPalette(QWidget* parent) : Palette(parent) {
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	show();

	brush.texture_operator.tile_id = map->terrain.tileset_ids.front();
	map->brush = &brush;

	change_mode_this = new QShortcut(Qt::Key_Space, this, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);
	change_mode_parent = new QShortcut(Qt::Key_Space, parent, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);

	// generate terrain/cliff texture buttons, and a blight button
	create_terrain_buttons();

	// ribbon
	create_ribbon();

	// setup operators
	setup_texture_operator();
	setup_cliff_operator();
	setup_deformation_operator();
	setup_cell_operator();

	// brush menu
	setup_brush_menu();
}

TerrainPalette::~TerrainPalette() {
	map->brush = nullptr;
	delete change_mode_parent;
	delete change_mode_this;
}

bool TerrainPalette::event(QEvent* e) {
	if (e->type() == QEvent::Close) {
		change_mode_this->setEnabled(false);
		change_mode_parent->setEnabled(false);
		ribbon_tab->setParent(nullptr);
		delete ribbon_tab;
	} else if (e->type() == QEvent::WindowActivate) {
		change_mode_this->setEnabled(true);
		change_mode_parent->setEnabled(true);
		map->brush = &brush;
		emit ribbon_tab_requested(ribbon_tab, "Terrain Palette");
	}
	return QWidget::event(e);
}

void TerrainPalette::deactivate(QRibbonTab* tab) {
	if (tab != ribbon_tab) {
		brush.clear_selection();
		change_mode_this->setEnabled(false);
		change_mode_parent->setEnabled(false);
	}
}

void TerrainPalette::update_operator_gui() {
	update_texture_operator_gui();
	update_cliff_operator_gui();
	update_deformation_operator_gui();
	update_cell_operator_gui();
}

void TerrainPalette::update_cell_operator_gui() {
	bool is_enabled = brush.cell_operator.is_enabled();

	// update checkbox state (disable events for the change)
	QSignalBlocker blocker(ui.cellCheckbox);
	ui.cellCheckbox->setChecked(is_enabled);

	// update cell operation buttons
	auto operation = brush.cell_operator.get_operation_type();
	{
		QSignalBlocker blocker(ui.addWater);
		ui.addWater->setChecked(is_enabled && operation == CellOperator::cell_operation::add_water);
	}
	{
		QSignalBlocker blocker(ui.removeWater);
		ui.removeWater->setChecked(is_enabled && operation == CellOperator::cell_operation::remove_water);
	}
	{
		QSignalBlocker blocker(ui.addBoundary);
		ui.addBoundary->setChecked(is_enabled && operation == CellOperator::cell_operation::add_boundary);
	}
	{
		QSignalBlocker blocker(ui.removeBoundary);
		ui.removeBoundary->setChecked(is_enabled && operation == CellOperator::cell_operation::remove_boundary);
	}
}

void TerrainPalette::update_deformation_operator_gui() {
	bool is_enabled = brush.height_operator.is_enabled();

	// update checkbox state (disable events for the change)
	QSignalBlocker blocker(ui.deformationCheckbox);
	ui.deformationCheckbox->setChecked(is_enabled);

	// update height change operation buttons
	auto deformation = brush.height_operator.deformation_type;
	{
		QSignalBlocker blocker(ui.terrainRaise);
		ui.terrainRaise->setChecked(is_enabled && deformation == HeightOperator::deformation::raise);
	}
	{
		QSignalBlocker blocker(ui.terrainLower);
		ui.terrainLower->setChecked(is_enabled && deformation == HeightOperator::deformation::lower);
	}
	{
		QSignalBlocker blocker(ui.terrainPlateau);
		ui.terrainPlateau->setChecked(is_enabled && deformation == HeightOperator::deformation::plateau);
	}
	{
		QSignalBlocker blocker(ui.terrainRipple);
		ui.terrainRipple->setChecked(is_enabled && deformation == HeightOperator::deformation::ripple);
	}
	{
		QSignalBlocker blocker(ui.terrainSmooth);
		ui.terrainSmooth->setChecked(is_enabled && deformation == HeightOperator::deformation::smooth);
	}
}

void TerrainPalette::update_cliff_operator_gui() {
	bool is_enabled = brush.cliff_operator.is_enabled();

	// update checkbox state (disable events for the change)
	QSignalBlocker blocker(ui.cliffCheckbox);
	ui.cliffCheckbox->setChecked(is_enabled);

	// update which cliff button is selected
	int cliff_id = brush.cliff_operator.cliff_id;
	for (auto* button : cliff_group->buttons()) {
		QSignalBlocker blocker(button);
		if (button->property("cliffID").toInt() == cliff_id) {
			button->setChecked(is_enabled);
		} else {
			button->setChecked(false);
		}
	}

	// update which operation is selected (i.e. ramp, level, deep water...)
	auto operation = brush.cliff_operator.cliff_operation_type;
	{
		QSignalBlocker blocker(ui.cliffLower2);
		ui.cliffLower2->setChecked(is_enabled && operation == CliffOperator::cliff_operation::lower2);
	}
	{
		QSignalBlocker blocker(ui.cliffLower1);
		ui.cliffLower1->setChecked(is_enabled && operation == CliffOperator::cliff_operation::lower1);
	}
	{
		QSignalBlocker blocker(ui.cliffLevel);
		ui.cliffLevel->setChecked(is_enabled && operation == CliffOperator::cliff_operation::level);
	}
	{
		QSignalBlocker blocker(ui.cliffRaise1);
		ui.cliffRaise1->setChecked(is_enabled && operation == CliffOperator::cliff_operation::raise1);
	}
	{
		QSignalBlocker blocker(ui.cliffRaise2);
		ui.cliffRaise2->setChecked(is_enabled && operation == CliffOperator::cliff_operation::raise2);
	}
	{
		QSignalBlocker blocker(ui.cliffDeepWater);
		ui.cliffDeepWater->setChecked(is_enabled && operation == CliffOperator::cliff_operation::deep_water);
	}
	{
		QSignalBlocker blocker(ui.cliffShallowWater);
		ui.cliffShallowWater->setChecked(is_enabled && operation == CliffOperator::cliff_operation::shallow_water);
	}
	{
		QSignalBlocker blocker(ui.cliffRamp);
		ui.cliffRamp->setChecked(is_enabled && operation == CliffOperator::cliff_operation::ramp);
	}
}

void TerrainPalette::update_texture_operator_gui() {
	bool is_enabled = brush.texture_operator.is_enabled();

	// update checkbox state (disable events for the change)
	QSignalBlocker blocker(ui.textureCheckbox);
	ui.textureCheckbox->setChecked(is_enabled);

	// update which texture button is selected
	QString tile_id = QString::fromStdString(brush.texture_operator.tile_id);
	for (auto* button : textures_group->buttons()) {
		QSignalBlocker blocker(button);
		if (button->property("tileID").toString() == tile_id) {
			button->setChecked(is_enabled);
		} else {
			button->setChecked(false);
		}
	}
}

void TerrainPalette::setup_texture_operator() {
	// activate/deactivate texture operator when the checkbox is clicked
	connect(ui.textureCheckbox, &QCheckBox::clicked, [&](bool checked) {
		if (checked) {
			brush.activate_operator(brush.texture_operator);
		} else {
			brush.deactivate_operator(brush.texture_operator);
		}

		update_operator_gui();
	});

	// when a texture icon is clicked on, we have to enable TextureOperator
	// and then select the proper ground texture
	connect(textures_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		// set the correct texture (blight included)
		brush.texture_operator.tile_id = button->property("tileID").toString().toStdString();

		// enable the operator
		// note that TerrainBrush will automatically deactivate all incompatible operators
		brush.activate_operator(brush.texture_operator);

		update_operator_gui();
	});
}

void TerrainPalette::setup_cliff_operator() {
	// allow the GUI to deselect all cliff operations
	ui.cliffButtonGroup->setExclusive(false);

	// activate/deactivate cliff operator when the checkbox is clicked
	connect(ui.cliffCheckbox, &QCheckBox::clicked, [&](bool checked) {
		if (checked) {
			brush.activate_operator(brush.cliff_operator);
		} else {
			brush.deactivate_operator(brush.cliff_operator);
		}

		update_operator_gui();
	});

	// cliff texture buttons
	connect(cliff_group, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		// update operator value and activate it
		// note that terrain brush will automatically deactivate all incompatible operators
		brush.cliff_operator.cliff_id = button->property("cliffID").toInt();
		brush.activate_operator(brush.cliff_operator);
		update_operator_gui();
	});

	// specific cliff operations
	connect(ui.cliffLower2, &QPushButton::clicked, [&]() {
		brush.cliff_operator.cliff_operation_type = CliffOperator::cliff_operation::lower2;
		brush.activate_operator(brush.cliff_operator);
		update_operator_gui();
	});

	connect(ui.cliffLower1, &QPushButton::clicked, [&]() {
		brush.cliff_operator.cliff_operation_type = CliffOperator::cliff_operation::lower1;
		brush.activate_operator(brush.cliff_operator);
		update_operator_gui();
	});

	connect(ui.cliffLevel, &QPushButton::clicked, [&]() {
		brush.cliff_operator.cliff_operation_type = CliffOperator::cliff_operation::level;
		brush.activate_operator(brush.cliff_operator);
		update_operator_gui();
	});

	connect(ui.cliffRaise1, &QPushButton::clicked, [&]() {
		brush.cliff_operator.cliff_operation_type = CliffOperator::cliff_operation::raise1;
		brush.activate_operator(brush.cliff_operator);
		update_operator_gui();
	});

	connect(ui.cliffRaise2, &QPushButton::clicked, [&]() {
		brush.cliff_operator.cliff_operation_type = CliffOperator::cliff_operation::raise2;
		brush.activate_operator(brush.cliff_operator);
		update_operator_gui();
	});

	connect(ui.cliffDeepWater, &QPushButton::clicked, [&]() {
		brush.cliff_operator.cliff_operation_type = CliffOperator::cliff_operation::deep_water;
		brush.activate_operator(brush.cliff_operator);
		update_operator_gui();
	});

	connect(ui.cliffShallowWater, &QPushButton::clicked, [&]() {
		brush.cliff_operator.cliff_operation_type = CliffOperator::cliff_operation::shallow_water;
		brush.activate_operator(brush.cliff_operator);
		update_operator_gui();
	});

	connect(ui.cliffRamp, &QPushButton::clicked, [&]() {
		brush.cliff_operator.cliff_operation_type = CliffOperator::cliff_operation::ramp;
		brush.activate_operator(brush.cliff_operator);
		update_operator_gui();
	});
}

void TerrainPalette::setup_cell_operator() {
	// allow the GUI to deselect all cell operations
	ui.cellButtonGroup->setExclusive(false);

	// setup operator
	connect(ui.cellCheckbox, &QCheckBox::clicked, [&](bool checked) {
		if (checked) {
			brush.activate_operator(brush.cell_operator);
		} else {
			brush.deactivate_operator(brush.cell_operator);
		}

		update_operator_gui();
	});

	// cell operation choice
	connect(ui.addWater, &QPushButton::clicked, [&]() {
		brush.cell_operator.set_operation_type(CellOperator::cell_operation::add_water);
		brush.activate_operator(brush.cell_operator);
		update_operator_gui();
	});
	connect(ui.removeWater, &QPushButton::clicked, [&]() {
		brush.cell_operator.set_operation_type(CellOperator::cell_operation::remove_water);
		brush.activate_operator(brush.cell_operator);
		update_operator_gui();
	});
	connect(ui.addBoundary, &QPushButton::clicked, [&]() {
		brush.cell_operator.set_operation_type(CellOperator::cell_operation::add_boundary);
		brush.activate_operator(brush.cell_operator);
		update_operator_gui();
	});
	connect(ui.removeBoundary, &QPushButton::clicked, [&]() {
		brush.cell_operator.set_operation_type(CellOperator::cell_operation::remove_boundary);
		brush.activate_operator(brush.cell_operator);
		update_operator_gui();
	});
}

void TerrainPalette::setup_deformation_operator() {
	// allow the GUI to deselect all deformation operations
	ui.deformationButtonGroup->setExclusive(false);

	// activate/deactivate height operator when the checkbox is clicked
	connect(ui.deformationCheckbox, &QCheckBox::clicked, [&](bool checked) {
		if (checked) {
			brush.activate_operator(brush.height_operator);
		} else {
			brush.deactivate_operator(brush.height_operator);
		}

		update_operator_gui();
	});

	// height change operations - set deformation type first, then activate
	connect(ui.terrainRaise, &QPushButton::clicked, [&]() {
		brush.height_operator.deformation_type = HeightOperator::deformation::raise;
		brush.activate_operator(brush.height_operator);
		update_operator_gui();
	});
	connect(ui.terrainLower, &QPushButton::clicked, [&]() {
		brush.height_operator.deformation_type = HeightOperator::deformation::lower;
		brush.activate_operator(brush.height_operator);
		update_operator_gui();
	});
	connect(ui.terrainPlateau, &QPushButton::clicked, [&]() {
		brush.height_operator.deformation_type = HeightOperator::deformation::plateau;
		brush.activate_operator(brush.height_operator);
		update_operator_gui();
	});
	connect(ui.terrainRipple, &QPushButton::clicked, [&]() {
		brush.height_operator.deformation_type = HeightOperator::deformation::ripple;
		brush.activate_operator(brush.height_operator);
		update_operator_gui();
	});
	connect(ui.terrainSmooth, &QPushButton::clicked, [&]() {
		brush.height_operator.deformation_type = HeightOperator::deformation::smooth;
		brush.activate_operator(brush.height_operator);
		update_operator_gui();
	});
}

void TerrainPalette::create_ribbon() {
	QRibbonSection* selection_section = new QRibbonSection;
	selection_section->setText("Selection");

	selection_mode->setText("Selection\nMode");
	selection_mode->setIcon(QIcon("data/icons/ribbon/select.png"));
	selection_mode->setCheckable(true);
	selection_section->addWidget(selection_mode);

	QRibbonSection* general_section = new QRibbonSection;
	general_section->setText("General");

	QRibbonButton* enforce_water_height_limit = new QRibbonButton;
	enforce_water_height_limit->setText("Enforce Water\nHeight Limit");
	enforce_water_height_limit->setIcon(QIcon("data/icons/ribbon/variation.png"));
	enforce_water_height_limit->setCheckable(true);
	enforce_water_height_limit->setChecked(true);
	general_section->addWidget(enforce_water_height_limit);

	QRibbonButton* change_doodad_heights = new QRibbonButton;
	change_doodad_heights->setText("Update\nDoodad Z");
	change_doodad_heights->setIcon(QIcon("data/icons/ribbon/changeheight.png"));
	change_doodad_heights->setCheckable(true);
	change_doodad_heights->setChecked(true);
	general_section->addWidget(change_doodad_heights);

	QRibbonSection* cliff_section = new QRibbonSection;
	cliff_section->setText("Cliff");

	QRibbonButton* relative_cliff_heights = new QRibbonButton;
	relative_cliff_heights->setText("Relative\nHeight");
	relative_cliff_heights->setIcon(QIcon("data/icons/ribbon/changeheight.png"));
	relative_cliff_heights->setCheckable(true);
	relative_cliff_heights->setChecked(false);
	relative_cliff_heights->setEnabled(false);
	cliff_section->addWidget(relative_cliff_heights);

	QRibbonSection* pathing_section = new QRibbonSection;
	pathing_section->setText("Pathing");

	QRibbonButton* apply_cliff_pathing = new QRibbonButton;
	apply_cliff_pathing->setText("Cliff\nPathing");
	apply_cliff_pathing->setIcon(QIcon("data/icons/ribbon/rock.png"));
	apply_cliff_pathing->setCheckable(true);
	apply_cliff_pathing->setChecked(true);
	pathing_section->addWidget(apply_cliff_pathing);

	QRibbonButton* apply_tile_pathing = new QRibbonButton;
	apply_tile_pathing->setText("Tile\nPathing");
	apply_tile_pathing->setIcon(QIcon("data/icons/ribbon/tileset.png"));
	apply_tile_pathing->setCheckable(true);
	apply_tile_pathing->setChecked(true);
	pathing_section->addWidget(apply_tile_pathing);

	QRibbonButton* apply_water_pathing = new QRibbonButton;
	apply_water_pathing->setText("Water\nPathing");
	apply_water_pathing->setIcon(QIcon("data/icons/ribbon/water.png"));
	apply_water_pathing->setCheckable(true);
	apply_water_pathing->setChecked(true);
	pathing_section->addWidget(apply_water_pathing);

	QRibbonSection* deformation_section = new QRibbonSection;
	deformation_section->setText("Deformation");

	QRibbonButton* deform_ground = new QRibbonButton;
	deform_ground->setText("Ground");
	deform_ground->setIcon(QIcon("data/icons/ribbon/heightmap.png"));
	deform_ground->setCheckable(true);
	deform_ground->setChecked(brush.deform_ground);
	deformation_section->addWidget(deform_ground);

	QRibbonButton* deform_water = new QRibbonButton;
	deform_water->setText("Water");
	deform_water->setIcon(QIcon("data/icons/ribbon/water.png"));
	deform_water->setCheckable(true);
	deform_water->setChecked(brush.deform_water);
	deformation_section->addWidget(deform_water);

	ribbon_tab->addSection(selection_section);
	ribbon_tab->addSection(general_section);
	ribbon_tab->addSection(cliff_section);
	ribbon_tab->addSection(pathing_section);
	ribbon_tab->addSection(deformation_section);

	connect(selection_mode, &QRibbonButton::toggled, [&]() {
		brush.switch_mode();
	});
	connect(change_mode_this, &QShortcut::activated, [&]() {
		selection_mode->click();
	});

	connect(change_mode_parent, &QShortcut::activated, [&]() {
		selection_mode->click();
	});

	connect(enforce_water_height_limit, &QRibbonButton::toggled, [&](bool checked) {
		brush.enforce_water_height_limits = checked;
	});
	connect(change_doodad_heights, &QRibbonButton::toggled, [&](bool checked) {
		brush.change_doodad_heights = checked;
	});
	connect(relative_cliff_heights, &QRibbonButton::toggled, [&](bool checked) {
		brush.relative_cliff_heights = checked;
	});

	connect(enforce_water_height_limit, &QRibbonButton::toggled, [&](bool checked) {
		brush.enforce_water_height_limits = checked;
	});

	connect(apply_cliff_pathing, &QRibbonButton::toggled, [&](bool checked) {
		brush.apply_cliff_pathing = checked;
	});
	connect(apply_tile_pathing, &QRibbonButton::toggled, [&](bool checked) {
		brush.apply_tile_pathing = checked;
	});
	connect(apply_water_pathing, &QRibbonButton::toggled, [&](bool checked) {
		brush.apply_water_pathing = checked;
	});

	connect(deform_ground, &QRibbonButton::toggled, [&](bool checked) {
		brush.deform_ground = checked;
	});
	connect(deform_water, &QRibbonButton::toggled, [&](bool checked) {
		brush.deform_water = checked;
	});
}

void TerrainPalette::setup_brush_menu() {
	connect(ui.brushSizeButtonGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), [&](QAbstractButton* button) {
		ui.brushSizeSlider->setValue(button->text().toInt());
	});
	connect(ui.brushSizeSlider, &QSlider::valueChanged, [&](int value) {
		brush.set_size(glm::ivec2(value));
		ui.brushSize->setValue(value);
	});

	connect(ui.deformationCheckbox, &QCheckBox::clicked, [&](bool checked) {
		if (checked) {
			brush.activate_operator(brush.height_operator);
		} else {
			brush.deactivate_operator(brush.height_operator);
		}
	});

	connect(ui.brushShapeCircle, &QPushButton::clicked, [&]() {
		brush.set_shape(Brush::Shape::circle);
	});
	connect(ui.brushShapeSquare, &QPushButton::clicked, [&]() {
		brush.set_shape(Brush::Shape::square);
	});
	connect(ui.brushShapeDiamond, &QPushButton::clicked, [&]() {
		brush.set_shape(Brush::Shape::diamond);
	});
}

void TerrainPalette::create_terrain_buttons() {
	ui.flowLayout_placeholder->addLayout(textures_layout);
	ui.flowLayout_placeholder_2->addLayout(cliff_layout);

	// allow button groups to have no buttons checked
	textures_group->setExclusive(false);
	cliff_group->setExclusive(false);

	// Ground Tiles
	const slk::SLK& slk = map->terrain.terrain_slk;
	for (const auto& i : map->terrain.tileset_ids) {
		const auto image = resource_manager.load<Texture>(slk.data("dir", i) + "/" + slk.data("file", i)).value();
		const auto icon = ground_texture_to_icon(image->data.data(), image->width, image->height);

		QPushButton* button =
			terrain_button(icon, "tileID", QString::fromStdString(i), QString::fromUtf8(slk.data<std::string_view>("comment", i)));

		textures_layout->addWidget(button);
		textures_group->addButton(button);

		// check if we are dealing with a cliff tile - if so, add it to cliff operator
		auto& cliff_tiles = map->terrain.cliff_to_ground_texture;
		const auto is_cliff_tile = std::ranges::find(cliff_tiles, map->terrain.ground_texture_to_id[i]);
		if (is_cliff_tile != cliff_tiles.end()) {
			const int index = std::distance(cliff_tiles.begin(), is_cliff_tile);

			button = terrain_button(icon, "cliffID", QString::number(index), QString::fromUtf8(slk.data<std::string_view>("comment", i)));
			cliff_layout->addWidget(button);
			cliff_group->addButton(button);
		}
	}

	// add blight texture to the texture painter tool
	// this is a small divergence from vanilla WE
	const auto image = resource_manager.load<Texture>("TerrainArt/Blight/Ashen_Blight.dds").value();
	const auto icon = ground_texture_to_icon(image->data.data(), image->width, image->height);

	QPushButton* blight_button = terrain_button(icon, "tileID", "blight", "Blight");
	textures_layout->addWidget(blight_button);
	textures_group->addButton(blight_button);
}

QPushButton*
TerrainPalette::terrain_button(const QIcon& icon, const char* propertyName, const QVariant& propertyValue, const QString& tileName) {
	QPushButton* button = new QPushButton;
	button->setIcon(icon);
	button->setFixedSize(48, 48);
	button->setIconSize({48, 48});
	button->setCheckable(true);
	button->setProperty(propertyName, propertyValue);
	button->setProperty("tileName", tileName);
	button->setToolTip(tileName);
	return button;
}
