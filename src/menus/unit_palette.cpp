#include "unit_palette.h"

#include <map_global.h>
#include <globals.h>

#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QAbstractProxyModel>
#include <QSortFilterProxyModel>


#include "table_model.h"

UnitPalette::UnitPalette(QWidget* parent) : Palette(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	show();

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

	QRibbonSection* selection_section = new QRibbonSection;
	selection_section->setText("Selection");

	selection_mode->setText("Selection\nMode");
	selection_mode->setIcon(QIcon("Data/Icons/Ribbon/select32x32.png"));
	selection_mode->setCheckable(true);
	selection_section->addWidget(selection_mode);

	selector = new UnitSelector(this);
	selector->setObjectName("selector");
	ui.verticalLayout->addWidget(selector);

	find_this = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);
	find_parent = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), parent, nullptr, nullptr, Qt::ShortcutContext::WindowShortcut);
	selection_mode->setShortCut(Qt::Key_Space, { this, parent });

	ribbon_tab->addSection(selection_section);
	
	connect(selection_mode, &QRibbonButton::toggled, [&]() { brush.switch_mode(); });

	connect(ui.player, QOverload<int>::of(&QComboBox::currentIndexChanged), [&]() {
		brush.player_id = ui.player->currentData().toInt();
	});

	connect(find_this, &QShortcut::activated, [&]() {
		activateWindow();
		selector->search->setFocus();
		selector->search->selectAll();
	});

	connect(find_parent, &QShortcut::activated, [&]() {
		activateWindow();
		selector->search->setFocus();
		selector->search->selectAll();
	});

	
	connect(selector, &UnitSelector::unitSelected, [&](const std::string& id) { 
		brush.set_unit(id); 
		selection_mode->setChecked(false);
	});
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

void UnitPalette::deactivate(QRibbonTab* tab) {
	if (tab != ribbon_tab) {
		brush.clear_selection();
		selection_mode->disableShortcuts();
		find_this->setEnabled(false);
		find_parent->setEnabled(false);
	}
}