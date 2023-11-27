#include "doodad_palette.h"

#include <print>

#include <QRadioButton>
#include <QCheckBox>
#include <QFormLayout>
#include <QDoubleValidator>
#include <QComboBox>
#include <QListView>
#include <QToolButton>
#include <QShortcut>
#include <QFrame>
#include <QGridLayout>
#include <QBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QTabWidget>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QMap>
#include <QScrollArea>
#include <QPushButton>
#include <QKeySequence>
#include <QTimer>

#include "globals.h"
#include <map_global.h>

#include <object_editor/object_editor.h>


import TableModel;
import QRibbon;

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
		ui.type->addItem(QString::fromStdString(value.front()), QString::fromStdString(key));
	}

	doodad_list_model = new DoodadListModel(this);
	doodad_list_model->setSourceModel(doodads_table);

	destructable_list_model = new DestructableListModel(this);
	destructable_list_model->setSourceModel(destructibles_table);

	doodad_filter_model = new DoodadListFilter(this);
	doodad_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	doodad_filter_model->setSourceModel(doodad_list_model);
	doodad_filter_model->sort(0, Qt::AscendingOrder);

	destructable_filter_model = new DestructableListFilter(this);
	destructable_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	destructable_filter_model->setSourceModel(destructable_list_model);
	destructable_filter_model->sort(0, Qt::AscendingOrder);

	concat_table = new QConcatenateTablesProxyModel(this);
	concat_table->addSourceModel(destructable_filter_model);
	concat_table->addSourceModel(doodad_filter_model);

	ui.doodads->setModel(concat_table);

	QRibbonSection* selection_section = new QRibbonSection;
	selection_section->setText("Selection");

	selection_mode->setText("Selection\nMode");
	selection_mode->setIcon(QIcon("data/icons/ribbon/select32x32.png"));
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

	find_this = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);
	find_parent = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), parent, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);

	change_mode_this = new QShortcut(Qt::Key_Space, this, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);
	change_mode_parent = new QShortcut(Qt::Key_Space, parent, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);

	QRibbonSection* placement_section = new QRibbonSection;
	placement_section->setText("Placement");

	QRibbonButton* random_rotation = new QRibbonButton;
	random_rotation->setText("Random\nRotation");
	random_rotation->setToolTip("Placed doodads will get a random rotation as long as their properties allow it");
	random_rotation->setIcon(QIcon("data/icons/ribbon/reset32x32.png"));
	random_rotation->setCheckable(true);
	random_rotation->setChecked(true);
	placement_section->addWidget(random_rotation);

	QRibbonButton* random_scale = new QRibbonButton;
	random_scale->setText("Random\nScale");
	random_scale->setToolTip("Placed doodads will get a random scale each time between their minScale and maxScale");
	random_scale->setIcon(QIcon("data/icons/ribbon/scale32x32.png"));
	random_scale->setCheckable(true);
	random_scale->setChecked(true);
	placement_section->addWidget(random_scale);

	QRibbonButton* lock_height = new QRibbonButton;
	lock_height->setText("Lock\nHeight");
	lock_height->setToolTip("Locks the absolute height of selected doodads when moved");
	lock_height->setIcon(QIcon("data/icons/ribbon/lock.png"));
	lock_height->setCheckable(true);
	placement_section->addWidget(lock_height);

	QRibbonSection* variation_section = new QRibbonSection;
	variation_section->setText("Variations");

	QRibbonButton* random_variation = new QRibbonButton;
	random_variation->setText("Random\nVariation");
	random_variation->setToolTip("Placed doodads will get a random variation. You can control which ones on the right ->");
	random_variation->setIcon(QIcon("data/icons/ribbon/variation32x32.png"));
	random_variation->setCheckable(true);
	random_variation->setChecked(true);
	variation_section->addWidget(random_variation);
	variation_section->addWidget(variations);

	QRibbonSection* flags_section = new QRibbonSection;
	flags_section->setText("Flags");

	QVBoxLayout* visibility_flags_layout = new QVBoxLayout;

	QRadioButton* invisible_non_solid = new QRadioButton;
	invisible_non_solid->setText("Invisible non solid");
	invisible_non_solid->setToolTip("Invisible and can units walk through");

	QRadioButton* visible_non_solid = new QRadioButton;
	visible_non_solid->setText("Visible non solid");
	visible_non_solid->setToolTip("Visible but units can walk through");

	QRadioButton* visible_solid = new QRadioButton;
	visible_solid->setText("Visible solid");
	visible_solid->setToolTip("Visible and units cannot walk through");
	visible_solid->setChecked(true);

	visibility_flags_layout->addWidget(invisible_non_solid);
	visibility_flags_layout->addWidget(visible_non_solid);
	visibility_flags_layout->addWidget(visible_solid);

	visibility_flags_layout->setSpacing(6);
	flags_section->addLayout(visibility_flags_layout);

	current_selection_section = new QRibbonSection;
	current_selection_section->setText("Current Selection");
	current_selection_section->setEnabled(false);

	QFormLayout* scaling_layout = new QFormLayout;
	scaling_layout->setSpacing(1);
	scaling_layout->setHorizontalSpacing(5);

	x_scale->setValidator(new QDoubleValidator(0.0, 100.0, 3));
	y_scale->setValidator(new QDoubleValidator(0.0, 100.0, 3));
	z_scale->setValidator(new QDoubleValidator(0.0, 100.0, 3));

	scaling_layout->addRow("x scale:", x_scale);
	scaling_layout->addRow("y scale:", y_scale);
	scaling_layout->addRow("z scale:", z_scale);

	QVBoxLayout* rotation_layout = new QVBoxLayout;

	QRibbonButton* degrees0 = new QRibbonButton;
	QRibbonButton* degrees90 = new QRibbonButton;
	QRibbonButton* degrees180 = new QRibbonButton;
	QRibbonButton* degrees270 = new QRibbonButton;

	degrees0->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
	degrees90->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
	degrees180->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
	degrees270->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);

	degrees0->setText("0");
	degrees90->setText("90");
	degrees180->setText("180");
	degrees270->setText("270");

	rotation->setValidator(new QDoubleValidator(0.0, 100.0, 3));

	QGridLayout* degrees_layout = new QGridLayout;
	degrees_layout->addWidget(degrees0, 0, 0);
	degrees_layout->addWidget(degrees90, 1, 0);
	degrees_layout->addWidget(degrees180, 0, 1);
	degrees_layout->addWidget(degrees270, 1, 1);

	QHBoxLayout* rotation_sub = new QHBoxLayout;
	rotation_sub->addWidget(new QLabel("Angle:"));
	rotation_sub->addSpacing(5);
	rotation_sub->addWidget(rotation);
	rotation_layout->addLayout(rotation_sub);
	rotation_layout->addLayout(degrees_layout);

	QRibbonButton* average_z = new QRibbonButton;
	average_z->setText("Group\nHeight  ");
	average_z->setIcon(QIcon("data/icons/ribbon/height.png"));
	average_z->addAction(group_height_minimum);
	average_z->addAction(group_height_average);
	average_z->addAction(group_height_maximum);
	average_z->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);

	QFormLayout* height_layout = new QFormLayout;
	height_layout->setSpacing(1);
	height_layout->setHorizontalSpacing(5);

	absolute_height->setValidator(new QDoubleValidator(-100.0, 100.0, 3));
	relative_height->setValidator(new QDoubleValidator(-100.0, 100.0, 3));

	height_layout->addRow("Absolute Height:", absolute_height);
	height_layout->addRow("Relative Height:", relative_height);

	QSmallRibbonButton* edit_in_oe = new QSmallRibbonButton;
	edit_in_oe->setText("Edit in OE");
	edit_in_oe->setIcon(QIcon("data/icons/ribbon/objecteditor32x32.png"));

	QSmallRibbonButton* select_in_palette = new QSmallRibbonButton;
	select_in_palette->setText("Select in Palette");
	select_in_palette->setToolTip("Or click the doodad with middle mouse button");
	select_in_palette->setIcon(QIcon("data/icons/ribbon/doodads32x32.png"));

	QVBoxLayout* info_layout = new QVBoxLayout;
	info_layout->addWidget(selection_name);
	info_layout->addWidget(edit_in_oe);
	info_layout->addWidget(select_in_palette);

	current_selection_section->addLayout(scaling_layout);
	current_selection_section->addSpacing(5);
	current_selection_section->addLayout(rotation_layout);
	current_selection_section->addSpacing(5);
	current_selection_section->addLayout(height_layout);
	current_selection_section->addWidget(average_z);
	current_selection_section->addSpacing(5);
	current_selection_section->addLayout(info_layout);

	ribbon_tab->addSection(selection_section);
	ribbon_tab->addSection(placement_section);
	ribbon_tab->addSection(variation_section);
	ribbon_tab->addSection(flags_section);
	ribbon_tab->addSection(current_selection_section);

	connect(selection_mode, &QRibbonButton::toggled, [&]() { brush.switch_mode(); });
	connect(random_rotation, &QRibbonButton::toggled, [&](bool checked) { brush.random_rotation = checked; });
	connect(random_scale, &QRibbonButton::toggled, [&](bool checked) { brush.random_scale = checked; });
	connect(random_variation, &QRibbonButton::toggled, [&](bool checked) { brush.random_variation = checked; });
	connect(lock_height, &QRibbonButton::toggled, [&](bool checked) { brush.lock_doodad_z = checked; });

	connect(invisible_non_solid, &QRadioButton::clicked, [&]() { brush.state = Doodad::State::invisible_non_solid; });
	connect(visible_non_solid, &QRadioButton::clicked, [&]() { brush.state = Doodad::State::visible_non_solid; });
	connect(visible_solid, &QRadioButton::clicked, [&]() { brush.state = Doodad::State::visible_solid; });

	connect(ui.type, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index) {
		// Possible Qt bug. Try swapping the two lines below and see if it crashes when selecting a tree and then swapping to a doodad category
		destructable_filter_model->setFilterCategory(ui.type->currentData().toString());	
		doodad_filter_model->setFilterCategory(ui.type->currentData().toString());
	});

	connect(ui.tileset, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index) {
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

	connect(change_mode_this, &QShortcut::activated, [&]() {
		selection_mode->click();
	});

	connect(change_mode_parent, &QShortcut::activated, [&]() {
		selection_mode->click();
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

	connect(&brush, &DoodadBrush::selection_changed, this, &DoodadPalette::update_selection_info);
	connect(&brush, &DoodadBrush::angle_changed, this, &DoodadPalette::update_selection_info);
	connect(&brush, &DoodadBrush::scale_changed, this, &DoodadPalette::update_selection_info);
	connect(&brush, &DoodadBrush::position_changed, this, &DoodadPalette::update_selection_info);
	connect(&brush, &DoodadBrush::request_doodad_select, this, &DoodadPalette::select_id_in_palette);

	connect(x_scale, &QLineEdit::textEdited, [&](const QString& text) { update_scale_change(0, text); });
	connect(y_scale, &QLineEdit::textEdited, [&](const QString& text) { update_scale_change(1, text); });
	connect(z_scale, &QLineEdit::textEdited, [&](const QString& text) { update_scale_change(2, text); });
	connect(x_scale, &QLineEdit::editingFinished, [&]() { update_scale_finish(0); });
	connect(y_scale, &QLineEdit::editingFinished, [&]() { update_scale_finish(1); });
	connect(z_scale, &QLineEdit::editingFinished, [&]() { update_scale_finish(2); });

	connect(rotation, &QLineEdit::textEdited, this, &DoodadPalette::update_rotation_change);
	connect(absolute_height, &QLineEdit::textEdited, this, &DoodadPalette::update_absolute_change);
	connect(relative_height, &QLineEdit::textEdited, this, &DoodadPalette::update_relative_change);

	connect(degrees0, &QRibbonButton::clicked, [&]() { set_selection_rotation(0.f); });
	connect(degrees90, &QRibbonButton::clicked, [&]() { set_selection_rotation(90.f); });
	connect(degrees180, &QRibbonButton::clicked, [&]() { set_selection_rotation(180.f); });
	connect(degrees270, &QRibbonButton::clicked, [&]() { set_selection_rotation(270.f); });

	connect(group_height_minimum, &QAction::triggered, this, &DoodadPalette::set_group_height_minimum);
	connect(group_height_average, &QAction::triggered, this, &DoodadPalette::set_group_height_average);
	connect(group_height_maximum, &QAction::triggered, this, &DoodadPalette::set_group_height_maximum);
	
	connect(edit_in_oe, &QSmallRibbonButton::clicked, [&]() {
		bool created;
		auto editor = window_handler.create_or_raise<ObjectEditor>(nullptr, created);
		const Doodad* doodad = *brush.selections.begin();
		if (destructibles_slk.row_headers.contains(doodad->id)) {
			editor->select_id(ObjectEditor::Category::destructible, doodad->id);
		} else {
			editor->select_id(ObjectEditor::Category::doodad, doodad->id);
		}
	});

	connect(select_in_palette, &QSmallRibbonButton::clicked, [&]() {
		const Doodad* doodad = *brush.selections.begin();
		select_id_in_palette(doodad->id);
	});

	// Default to Trees/Destructibles
	ui.type->setCurrentIndex(ui.type->count() - 2);
	ui.tileset->setCurrentIndex(0);

	ui.search->setFocus();
	ui.search->selectAll();

	ui.doodads->setCurrentIndex(ui.doodads->model()->index(0, 0));
	selection_changed(ui.doodads->model()->index(0, 0));
}

DoodadPalette::~DoodadPalette() {
	map->brush = nullptr;
}

bool DoodadPalette::event(QEvent *e) {
	if (e->type() == QEvent::Close) {
		// Remove shortcut from parent
		find_this->setEnabled(false);
		find_parent->setEnabled(false);
		change_mode_this->setEnabled(false);
		change_mode_parent->setEnabled(false);
		ribbon_tab->setParent(nullptr);
		delete ribbon_tab;
	} else if (e->type() == QEvent::WindowActivate) {
		find_this->setEnabled(true);
		find_parent->setEnabled(true);
		change_mode_this->setEnabled(true);
		change_mode_parent->setEnabled(true);
		map->brush = &brush;
		emit ribbon_tab_requested(ribbon_tab, "Doodad Palette");
	}

	return QWidget::event(e);
}

void DoodadPalette::selection_changed(const QModelIndex& index) {
	std::string id;

	const auto model = concat_table->mapToSource(index).model();
	if (model == destructable_filter_model) {
		const int row = destructable_filter_model->mapToSource(concat_table->mapToSource(index)).row();
		id = destructibles_slk.index_to_row.at(row);
	} else if (model == doodad_filter_model) {
		const int row = doodad_filter_model->mapToSource(concat_table->mapToSource(index)).row();
		id = doodads_slk.index_to_row.at(row);
	}

	brush.set_doodad(id);
	selection_mode->setChecked(false);

	bool is_doodad = doodads_slk.row_headers.contains(id);
	slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

	variations->clear();

	int variation_count = slk.data<int>("numvar", id);
	for (int i = 0; i < variation_count; i++) {
		QRibbonButton* toggle = new QRibbonButton;
		toggle->setCheckable(true);
		toggle->setChecked(true);
		toggle->setText(QString::number(i));
		variations->addWidget(toggle, i % 3, i / 3);
		connect(toggle, &QRibbonButton::toggled, [&](bool checked) {
			if (checked) {
				brush.add_variation(i);
			} else {
				brush.erase_variation(i);
			}
		});
	}
}

void DoodadPalette::select_id_in_palette(std::string id) {
	ui.search->clear();
	
	if (destructibles_slk.row_headers.contains(id)) {
		const auto category = destructibles_slk.data("category", id);
		ui.type->setCurrentIndex(ui.type->findData(QString::fromStdString(category)));
		const auto index = destructable_filter_model->mapFromSource(destructable_list_model->mapFromSource(destructibles_table->rowIDToIndex(id)));
		const auto finally = concat_table->mapFromSource(index);
		ui.doodads->setCurrentIndex(finally);
		selection_changed(finally);
	} else {
		const auto category = doodads_slk.data("category", id);
		ui.type->setCurrentIndex(ui.type->findData(QString::fromStdString(category)));
		const auto index = doodad_filter_model->mapFromSource(doodad_list_model->mapFromSource(doodads_table->rowIDToIndex(id)));
		const auto finally = concat_table->mapFromSource(index);
		ui.doodads->setCurrentIndex(finally);
		selection_changed(finally);
	}
}

void DoodadPalette::deactivate(QRibbonTab* tab) {
	if (tab != ribbon_tab) {
		brush.clear_selection();
		selection_mode->disableShortcuts();
		find_this->setEnabled(false);
		find_parent->setEnabled(false);
		change_mode_this->setEnabled(false);
		change_mode_parent->setEnabled(false);
	}
}

QString toString(float num) {
	QString str = QString::number(num, 'f', 3);
	str.remove(QRegularExpression("\\.?0+$"));
	return str;
}

void DoodadPalette::update_selection_info() {
	if (brush.selections.empty()) {
		if (current_selection_section->isEnabled()) {
			current_selection_section->setEnabled(false);
		}
		selection_name->setText("");
	} else {
		if (!current_selection_section->isEnabled()) {
			current_selection_section->setEnabled(true);
		}
		const Doodad& doodad = **brush.selections.begin();

		float first_relative_height = doodad.position.z - map->terrain.interpolated_height(doodad.position.x, doodad.position.y, true);
		bool same_object = true;
		bool same_x = true;
		bool same_y = true;
		bool same_z = true;
		bool same_angle = true;
		bool same_absolute_height = true;
		bool same_relative_height = true;
		for (const auto& i : brush.selections) {
			float other_relative_height = i->position.z - map->terrain.interpolated_height(i->position.x, i->position.y, true);

			same_object = same_object && i->id == doodad.id;
			same_x = same_x && i->scale.x == doodad.scale.x;
			same_y = same_y && i->scale.y == doodad.scale.y;
			same_z = same_z && i->scale.z == doodad.scale.z;
			same_angle = same_angle && i->angle == doodad.angle;
			same_absolute_height = same_absolute_height && std::abs(i->position.z - doodad.position.z) < 0.001f;
			same_relative_height = same_relative_height && std::abs(other_relative_height - first_relative_height) < 0.001f;
		}

		x_scale->setText(same_x ? QString::number(doodad.scale.x) : "Differing");
		y_scale->setText(same_y ? QString::number(doodad.scale.y) : "Differing");
		z_scale->setText(same_z ? QString::number(doodad.scale.z) : "Differing");
		rotation->setText(same_angle ? toString(glm::degrees(doodad.angle)) : "Differing");
		absolute_height->setText(same_absolute_height ? toString(doodad.position.z) : "Differing");
		relative_height->setText(same_relative_height ? toString(first_relative_height) : "Differing");

		// Set the name
		if (same_object) {
			if (doodads_slk.row_headers.contains(doodad.id)) {
				auto index = doodads_table->index(doodads_slk.row_headers.at(doodad.id), doodads_slk.column_headers.at("name"));
				selection_name->setText(doodads_table->data(index).toString());
			} else {
				auto index = destructibles_table->index(destructibles_slk.row_headers.at(doodad.id), destructibles_slk.column_headers.at("name"));
				selection_name->setText(destructibles_table->data(index).toString());
			}
		} else {
			selection_name->setText("Various");
		}
	}
}

void DoodadPalette::update_scale_change(int component, const QString& text) {
	brush.set_selection_scale_component(component, text.toFloat());
}

void DoodadPalette::update_scale_finish(int component) {
	if (brush.selections.empty()) {
		return;
	}

	update_selection_info();
}

void DoodadPalette::update_rotation_change(const QString& text) {
	brush.set_selection_angle(glm::radians(text.toFloat()));
	update_selection_info();
}

void DoodadPalette::update_absolute_change(const QString& text) {
	brush.set_selection_absolute_height(text.toFloat());
	update_selection_info();
}

void DoodadPalette::update_relative_change(const QString& text) {
	brush.set_selection_relative_height(text.toFloat());
	update_selection_info();
}

void DoodadPalette::set_group_height_minimum() {
	float minimum = std::numeric_limits<float>::max();
	for (auto& i : brush.selections) {
		minimum = std::min(minimum, i->position.z);
	}

	brush.set_selection_absolute_height(minimum);
}

void DoodadPalette::set_group_height_average() {
	float average = 0.f;
	for (auto& i : brush.selections) {
		average += i->position.z;
	}
	brush.set_selection_absolute_height(average / brush.selections.size());
}

void DoodadPalette::set_group_height_maximum() {
	float maximum = std::numeric_limits<float>::min();
	for (auto& i : brush.selections) {
		maximum = std::max(maximum, i->position.z);
	}

	brush.set_selection_absolute_height(maximum);
}

// new_rotation in degrees
void DoodadPalette::set_selection_rotation(float new_rotation) {
	brush.set_selection_angle(glm::radians(new_rotation));
	update_selection_info();
}