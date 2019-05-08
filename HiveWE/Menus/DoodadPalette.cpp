#include "stdafx.h"

DoodadPalette::DoodadPalette(QWidget* parent) : Palette(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	show();

	brush.create();
	map->brush = &brush;

	for (auto&&[key, value] : world_edit_data.section("TileSets")) {
		const std::string tileset_key = value.front();
		ui.tileset->addItem(QString::fromStdString(tileset_key), QString::fromStdString(key));
	}

	for (auto&&[key, value] : world_edit_data.section("DoodadCategories")) {
		const std::string tileset_key = value.front();
		ui.type->addItem(QString::fromStdString(tileset_key), QString::fromStdString(key));
	}

	for (auto&&[key, value] : world_edit_data.section("DestructibleCategories")) {
		const std::string text = value.front();
		ui.type->addItem(QString::fromStdString(text), QString::fromStdString(key));
	}
	// Default to Trees/Destructibles
	ui.type->setCurrentIndex(ui.type->count() - 2);

	QRibbonSection* selection_section = new QRibbonSection;

	selection_mode->setText("Selection\nMode");
	selection_mode->setIcon(QIcon("Data/Icons/Ribbon/select32x32.png"));
	selection_mode->setCheckable(true);
	selection_section->addWidget(selection_mode);
	
	selection_mode->setShortCut(Qt::Key_Space, { this, parent });

	QRibbonSection* placement_section = new QRibbonSection;
	placement_section->setText("Placement");

	QRibbonButton* random_rotation = new QRibbonButton;
	random_rotation->setText("Random\nRotation");
	random_rotation->setIcon(QIcon("Data/Icons/Ribbon/reset32x32.png"));
	random_rotation->setCheckable(true);
	random_rotation->setChecked(true);
	placement_section->addWidget(random_rotation);

	QRibbonButton* random_scale = new QRibbonButton;
	random_scale->setText("Random\nScale");
	random_scale->setIcon(QIcon("Data/Icons/Ribbon/scale32x32.png"));
	random_scale->setCheckable(true);
	random_scale->setChecked(true);
	placement_section->addWidget(random_scale);

	QRibbonSection* variation_section = new QRibbonSection;
	variation_section->setText("Variations");

	QRibbonButton* random_variation = new QRibbonButton;
	random_variation->setText("Random\nVariation");
	random_variation->setIcon(QIcon("Data/Icons/Ribbon/variation32x32.png"));
	random_variation->setCheckable(true);
	random_variation->setChecked(true);
	variation_section->addWidget(random_variation);
	variation_section->addWidget(variations);

	QRibbonSection* flags_section = new QRibbonSection;
	flags_section->setText("Flags");
	flags_section->setStyleSheet(R"(
		QDoubleSpinBox {
			border: 1px solid black;
		}
	)");

	QVBoxLayout* visibility_flags_layout = new QVBoxLayout;

	QRadioButton* invisible_non_solid = new QRadioButton;
	invisible_non_solid->setText("Invisible non solid");

	QRadioButton* visible_non_solid = new QRadioButton;
	visible_non_solid->setText("Visible non solid");

	QRadioButton* visible_solid = new QRadioButton;
	visible_solid->setText("Visible solid");
	visible_solid->setChecked(true);

	visibility_flags_layout->addWidget(invisible_non_solid);
	visibility_flags_layout->addWidget(visible_non_solid);
	visibility_flags_layout->addWidget(visible_solid);

	visibility_flags_layout->setSpacing(6);
	flags_section->addLayout(visibility_flags_layout);


	pathing_section->setText("Pathing");
	pathing_section->addWidget(pathing_image_label);


	/*QVBoxLayout* lay = new QVBoxLayout;
	QDoubleSpinBox* but = new QDoubleSpinBox;
	QDoubleSpinBox* butt = new QDoubleSpinBox;

	but-> setStyleSheet(R"(
		QDoubleSpinBox {
			border: 1px solid black;
		}
	)");

	butt->setStyleSheet(R"(
		QDoubleSpinBox {
			border: 1px solid black;
		}
	)");

	QHBoxLayout* tt = new QHBoxLayout;
	tt->addWidget(new QLabel("Min"));
	tt->addWidget(but);

	QHBoxLayout* ttt = new QHBoxLayout;
	ttt->addWidget(new QLabel("Max"));
	ttt->addWidget(butt);

	ttt->setSpacing(6);*/

	//lay->addLayout(tt);
	//lay->addLayout(ttt);
	//lay->addWidget(buttt);

	//section->addLayout(lay);

	ribbon_tab->addSection(selection_section);
	ribbon_tab->addSection(placement_section);
	ribbon_tab->addSection(variation_section);
	ribbon_tab->addSection(flags_section);
	ribbon_tab->addSection(pathing_section);

	connect(selection_mode, &QRibbonButton::toggled, [&]() { brush.switch_mode(); });
	connect(random_rotation, &QRibbonButton::toggled, [&](bool checked) { brush.random_rotation = checked; });
	connect(random_scale, &QRibbonButton::toggled, [&](bool checked) { brush.random_scale = checked; });
	connect(random_variation, &QRibbonButton::toggled, [&](bool checked) { brush.random_variation = checked; });

	connect(invisible_non_solid, &QRadioButton::clicked, [&]() { brush.state = Doodad::State::invisible_non_solid; });
	connect(visible_non_solid, &QRadioButton::clicked, [&]() { brush.state = Doodad::State::visible_non_solid; });
	connect(visible_solid, &QRadioButton::clicked, [&]() { brush.state = Doodad::State::visible_solid; });

	connect(ui.tileset, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DoodadPalette::update_list);
	connect(ui.type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DoodadPalette::update_list);
	connect(ui.doodads, &QListWidget::itemClicked, this, &DoodadPalette::selection_changed);

	update_list();
}

DoodadPalette::~DoodadPalette() {
	map->brush = nullptr;
}

bool DoodadPalette::event(QEvent *e) {
	if (e->type() == QEvent::Close) {
		// Remove shortcut from parent
		selection_mode->disconnectShortcuts();
		ribbon_tab->setParent(nullptr);
		delete ribbon_tab;
	} else if (e->type() == QEvent::WindowActivate) {
		selection_mode->enableShortcuts();
		map->brush = &brush;
		emit ribbon_tab_requested(ribbon_tab, "Doodad Palette");
	}

	return QWidget::event(e);
}

void DoodadPalette::update_list() {
	ui.doodads->clear();
	
	char selected_tileset = ui.tileset->currentData().toString().toStdString().front();
	std::string selected_category = ui.type->currentData().toString().toStdString();

	bool is_doodad = world_edit_data.key_exists("DoodadCategories", selected_category);
	slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

	for (int i = 1; i < slk.rows; i++) {
		// If the doodad belongs to this tileset
		std::string tilesets = slk.data("tilesets", i);
		if (tilesets != "*" && tilesets.find(selected_tileset) == std::string::npos) {
			continue;
		}

		// If the doodad belongs to this category
		std::string category = slk.data("category", i);
		if (category != selected_category) {
			continue;
		}



		std::string text = slk.data("Name", i);

		const std::string trigstr = map->trigger_strings.string(text);
		if (!trigstr.empty()) {
			text = trigstr;
		}

		if (!is_doodad) { 
			text += " " + destructibles_slk.data("EditorSuffix", i);
		}

		QListWidgetItem* item = new QListWidgetItem(ui.doodads);
		item->setText(QString::fromStdString(text));
		item->setData(Qt::UserRole, QString::fromStdString(slk.data(is_doodad ? "doodID" : "DestructableID", i)));
	}
}

void DoodadPalette::selection_changed(QListWidgetItem* item) {
	const std::string id = item->data(Qt::UserRole).toString().toStdString();

	brush.set_doodad(id);
	selection_mode->setChecked(false);

	bool is_doodad = doodads_slk.row_header_exists(id);
	slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

	variations->clear();

	int variation_count = slk.data<int>("numVar", id);
	for (int i = 0; i < variation_count; i++) {
		QRibbonButton* toggle = new QRibbonButton;
		toggle->setCheckable(true);
		toggle->setChecked(true);
		toggle->setText(QString::number(i));
		variations->addWidget(toggle, i % 3, i / 3);
		connect(toggle, &QRibbonButton::toggled, [=](bool checked) {
			if (checked) {
				brush.add_variation(i);
			} else {
				brush.erase_variation(i);
			}
		});
	}

	if (brush.pathing_texture) {
		pathing_section->setHidden(false);
		QImage::Format format = brush.pathing_texture->channels == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_RGBA8888;
		QImage temp_image = QImage(brush.pathing_texture->data.data(), brush.pathing_texture->width, brush.pathing_texture->height, brush.pathing_texture->width * brush.pathing_texture->channels, format);
		pathing_image_label->setPixmap(QPixmap::fromImage(temp_image));
	} else {
		pathing_section->setHidden(true);
	}
}

void DoodadPalette::deactivate(QRibbonTab* tab) {
	if (tab != ribbon_tab) {
		brush.clear_selection();
		selection_mode->disableShortcuts();
	}
}