#include "DoodadPalette.h"

#include <QRadioButton>
#include <QCheckBox>

#include "HiveWE.h"
#include "Selections.h"

#include "TableModel.h"

DoodadPalette::DoodadPalette(QWidget* parent) : Palette(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	show();

	map->brush = &brush;

	ui.tileset->addItem("All Tilesets", '*');
	for (auto&&[key, value] : world_edit_data.section("TileSets")) {
		ui.tileset->addItem(QString::fromStdString(value.front()), key.front());
	}

	for (auto&&[key, value] : world_edit_data.section("DoodadCategories")) {
		ui.type->addItem(QString::fromStdString(value.front()), QString::fromStdString(key));
	}

	for (auto&&[key, value] : world_edit_data.section("DestructibleCategories")) {
		const std::string text = value.front();
		ui.type->addItem(QString::fromStdString(text), QString::fromStdString(key));
	}

	DoodadListModel* doodad_list_model = new DoodadListModel;
	doodad_list_model->setSourceModel(doodads_table);

	DestructableListModel* destructable_list_model = new DestructableListModel;
	destructable_list_model->setSourceModel(destructables_table);

	doodad_filter_model = new DoodadListFilter(this);
	doodad_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	doodad_filter_model->setSourceModel(doodad_list_model);
	doodad_filter_model->sort(0, Qt::AscendingOrder);

	destructable_filter_model = new DestructableListFilter(this);
	destructable_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	destructable_filter_model->setSourceModel(destructable_list_model);
	destructable_filter_model->sort(0, Qt::AscendingOrder);

	table = new QConcatenateTablesProxyModel;
	table->addSourceModel(doodad_filter_model);
	table->addSourceModel(destructable_filter_model);

	ui.doodads->setModel(table);

	QRibbonSection* selection_section = new QRibbonSection;
	selection_section->setText("Selection");

	selection_mode->setText("Selection\nMode");
	selection_mode->setIcon(QIcon("Data/Icons/Ribbon/select32x32.png"));
	selection_mode->setCheckable(true);
	selection_section->addWidget(selection_mode);

	//selections_button->setText("View\nSelections");
	//selections_button->setIcon(QIcon("Data/Icons/Ribbon/description32x32.png.png"));
	//selection_section->addWidget(selections_button);

	//QVBoxLayout* selection_choices_layout = new QVBoxLayout;
	//QCheckBox* select_destructibles = new QCheckBox("Destructibles");
	//select_destructibles->setChecked(true);
	//QCheckBox* select_doodads = new QCheckBox("Doodads");
	//select_doodads->setChecked(true);
	
	//selection_choices_layout->addWidget(select_destructibles);
	//selection_choices_layout->addWidget(select_doodads);

	//selection_section->addLayout(selection_choices_layout);

	find_this = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), this, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);
	find_parent = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), parent, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);
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

	connect(ui.type, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
		// Possible Qt bug. Try swapping the two lines below and see if it crashes when selecting a tree and then swapping to a doodad category
		destructable_filter_model->setFilterCategory(ui.type->currentData().toString());
		doodad_filter_model->setFilterCategory(ui.type->currentData().toString());
	});

	connect(ui.tileset, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
		destructable_filter_model->setFilterTileset(ui.tileset->currentData().toChar().toLatin1());
		doodad_filter_model->setFilterTileset(ui.tileset->currentData().toChar().toLatin1());
	});

	connect(find_this, &QShortcut::activated, [&]() {
		ui.search->activateWindow();
		ui.search->setFocus();
		ui.search->selectAll();
	});

	connect(find_parent, &QShortcut::activated, [&]() {
		ui.search->activateWindow();
		ui.search->setFocus();
		ui.search->selectAll();
	});

	connect(ui.search, &QLineEdit::textEdited, doodad_filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(ui.search, &QLineEdit::textEdited, destructable_filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(ui.search, &QLineEdit::returnPressed, [&]() {
		ui.doodads->setCurrentIndex(ui.doodads->model()->index(0, 0));
		selection_changed(ui.doodads->model()->index(0, 0));
		ui.doodads->setFocus();
	});

	connect(ui.doodads, &QListView::clicked, this, &DoodadPalette::selection_changed);
	connect(ui.doodads, &QListView::activated, this, &DoodadPalette::selection_changed);

	// Default to Trees/Destructibles
	ui.type->setCurrentIndex(ui.type->count() - 2);
	ui.tileset->setCurrentIndex(0);

	ui.search->setFocus();
	ui.search->selectAll();
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

void DoodadPalette::selection_changed(const QModelIndex& index) {
	std::string id;

	const auto model = table->mapToSource(index).model();
	if (model == destructable_filter_model) {
		const int row = destructable_filter_model->mapToSource(table->mapToSource(index)).row();
		id = destructables_slk.index_to_row.at(row);
	} else if (model == doodad_filter_model) {
		const int row = doodad_filter_model->mapToSource(table->mapToSource(index)).row();
		id = doodads_slk.index_to_row.at(row);
	}

	brush.set_doodad(id);
	selection_mode->setChecked(false);

	bool is_doodad = doodads_slk.row_headers.contains(id);
	slk::SLK2& slk = is_doodad ? doodads_slk : destructables_slk;

	variations->clear();

	int variation_count = slk.data<int>("numvar", id);
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