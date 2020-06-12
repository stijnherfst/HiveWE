#include "UnitPalette.h"

#include <QComboBox>
#include <QLineEdit>
#include <QListView>

#include "HiveWE.h"
#include "Selections.h"

#include "TableModel.h"
#include <QSortFilterProxyModel>

UnitPalette::UnitPalette(QWidget* parent) : Palette(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	show();

	list_model = new UnitListModel(this);
	list_model->setSourceModel(units_table);

	filter_model = new UnitListFilter(this);
	filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	filter_model->setSourceModel(list_model);
	filter_model->sort(0, Qt::AscendingOrder);

	ui.units->setModel(filter_model);

	for (const auto& player : map->info.players) {
		std::string color_lookup = std::to_string(player.internal_number);
		if (color_lookup.size() == 1) {
			color_lookup = "0" + color_lookup;
		}

		std::string player_name = map->trigger_strings.string(player.name) + " (" + world_edit_strings.data("WorldEditStrings", "WESTRING_UNITCOLOR_" + color_lookup) + ")";

		ui.player->addItem(QString::fromStdString(player_name), player.internal_number);
	}
	ui.player->addItem("Neutral Hostile", 24);
	ui.player->addItem("Neutral Passive", 27);

	for (const auto& [key, value] : unit_editor_data.section("unitRace")) {
		if (key == "Sort" || key == "NumValues") {
			continue;
		}
		ui.race->addItem(QString::fromStdString(value[1]), QString::fromStdString(value[0]));
	}

	QRibbonSection* selection_section = new QRibbonSection;
	selection_section->setText("Selection");

	selection_mode->setText("Selection\nMode");
	selection_mode->setIcon(QIcon("Data/Icons/Ribbon/select32x32.png"));
	selection_mode->setCheckable(true);
	selection_section->addWidget(selection_mode);

	find_this = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), this, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);
	find_parent = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), parent, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);
	selection_mode->setShortCut(Qt::Key_Space, { this, parent });

	ribbon_tab->addSection(selection_section);
	
	connect(selection_mode, &QRibbonButton::toggled, [&]() { brush.switch_mode(); });

	connect(ui.player, QOverload<int>::of(&QComboBox::currentIndexChanged), [&]() {
		brush.player_id = ui.player->currentData().toInt();
	});
	connect(ui.race, QOverload<int>::of(&QComboBox::currentIndexChanged), [&]() {
		filter_model->setFilterRace(ui.race->currentData().toString());
	});

	connect(ui.search, &QLineEdit::textEdited, filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(ui.search, &QLineEdit::returnPressed, [&]() {
		ui.units->setCurrentIndex(ui.units->model()->index(0, 0));
		selection_changed(ui.units->model()->index(0, 0));
		ui.units->setFocus();
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

	connect(ui.units, &QListView::clicked, this, &UnitPalette::selection_changed);
	connect(ui.units, &QListView::activated, this, &UnitPalette::selection_changed);
}

UnitPalette::~UnitPalette() {
	map->brush = nullptr;
}

bool UnitPalette::event(QEvent* e) {
	if (e->type() == QEvent::Close) {
		// Remove shortcut from parent
		find_this->setEnabled(false);
		find_parent->setEnabled(false);
		selection_mode->disconnectShortcuts();
		ribbon_tab->setParent(nullptr);
		delete ribbon_tab;
	} else if (e->type() == QEvent::WindowActivate) {
		find_this->setEnabled(true);
		find_parent->setEnabled(true);
		selection_mode->enableShortcuts();
		map->brush = &brush;
		emit ribbon_tab_requested(ribbon_tab, "Unit Palette");
	}
	return QWidget::event(e);
}

void UnitPalette::selection_changed(const QModelIndex& item) {
	if (!item.isValid()) {
		return;
	}

	const int row = filter_model->mapToSource(item).row();
	brush.set_unit(units_slk.index_to_row.at(row));
}

void UnitPalette::deactivate(QRibbonTab* tab) {
	if (tab != ribbon_tab) {
		brush.clear_selection();
		selection_mode->disableShortcuts();
		find_this->setEnabled(false);
		find_parent->setEnabled(false);
	}
}