#include "UnitPalette.h"

#include <QComboBox>
#include <QLineEdit>

#include "HiveWE.h"
#include "Selections.h"

#include "UnitListModel.h"
#include "TableModel.h"
#include <QSortFilterProxyModel>

UnitPalette::UnitPalette(QWidget* parent) : Palette(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	show();

	TableModel* model = new TableModel(&units_slk, &units_meta_slk);

	UnitListModel* list_model = new UnitListModel;
	list_model->setSourceModel(model);

	UnitListFilter* filter_model = new UnitListFilter;
	filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	filter_model->setSourceModel(list_model);
	filter_model->sort(0, Qt::AscendingOrder);

	ui.units->setModel(filter_model);

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

	selection_mode->setShortCut(Qt::Key_Space, { this, parent });

	ribbon_tab->addSection(selection_section);

	connect(ui.race, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() {
		filter_model->setFilterRace(ui.race->currentData().toString());
	});
	connect(ui.search, &QLineEdit::textEdited, filter_model, &QSortFilterProxyModel::setFilterFixedString);
}

UnitPalette::~UnitPalette() {
	map->brush = nullptr;
}

bool UnitPalette::event(QEvent* e) {
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
	//return true;
}

void UnitPalette::deactivate(QRibbonTab* tab) {
	//if (tab != ribbon_tab) {
	//	brush.clear_selection();
	//	selection_mode->disableShortcuts();
	//}
}