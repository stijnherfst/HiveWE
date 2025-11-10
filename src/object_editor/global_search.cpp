#include "global_search.h"

#include "object_editor.h"

#include <QLayout>
#include <QFrame>
#include <QDialog>
#include <QSortFilterProxyModel>
#include <ui_object_editor.h>

import std;
import WindowHandler;
import Globals;
import TableModel;

GlobalSearchWidget::GlobalSearchWidget(QWidget* parent) : QDialog(parent) {
	setWindowFlag(Qt::FramelessWindowHint, true);
	setAttribute(Qt::WA_DeleteOnClose);

	resize(400, 700);

	ability_list_model = new AbilityListModel(this);
	ability_list_model->setSourceModel(abilities_table);
	ability_filter_model = new AbilityListFilter(this);
	ability_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ability_filter_model->setSourceModel(ability_list_model);
	ability_filter_model->sort(0, Qt::AscendingOrder);

	destructable_list_model = new DestructableListModel(this);
	destructable_list_model->setSourceModel(destructibles_table);
	destructable_filter_model = new DestructableListFilter(this);
	destructable_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	destructable_filter_model->setSourceModel(destructable_list_model);
	destructable_filter_model->sort(0, Qt::AscendingOrder);

	doodad_list_model = new DoodadListModel(this);
	doodad_list_model->setSourceModel(doodads_table);
	doodad_filter_model = new DoodadListFilter(this);
	doodad_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	doodad_filter_model->setSourceModel(doodad_list_model);
	doodad_filter_model->sort(0, Qt::AscendingOrder);

	items_list_model = new ItemListModel(this);
	items_list_model->setSourceModel(items_table);
	item_filter_model = new ItemListFilter(this);
	item_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	item_filter_model->setSourceModel(items_list_model);
	item_filter_model->sort(0, Qt::AscendingOrder);

	unit_list_model = new UnitListModel(this);
	unit_list_model->setSourceModel(units_table);
	units_filter_model = new UnitListFilter(this);
	units_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	units_filter_model->setSourceModel(unit_list_model);
	units_filter_model->sort(0, Qt::AscendingOrder);

	upgrade_list_model = new UpgradeListModel(this);
	upgrade_list_model->setSourceModel(upgrade_table);
	upgrade_filter_model = new UpgradeListFilter(this);
	upgrade_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	upgrade_filter_model->setSourceModel(upgrade_list_model);
	upgrade_filter_model->sort(0, Qt::AscendingOrder);

	buff_list_model = new BuffListModel(this);
	buff_list_model->setSourceModel(buff_table);
	buff_filter_model = new BuffListFilter(this);
	buff_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	buff_filter_model->setSourceModel(buff_list_model);
	buff_filter_model->sort(0, Qt::AscendingOrder);

	concat_table = new QConcatenateTablesProxyModel(this);
	concat_table->addSourceModel(units_filter_model);
	concat_table->addSourceModel(ability_filter_model);
	concat_table->addSourceModel(destructable_filter_model);
	concat_table->addSourceModel(doodad_filter_model);
	concat_table->addSourceModel(item_filter_model);
	concat_table->addSourceModel(upgrade_filter_model);
	concat_table->addSourceModel(buff_filter_model);

	list = new QListView;
	list->setIconSize(QSize(32, 32));
	list->setModel(concat_table);
	list->setUniformItemSizes(true);
	list->setCurrentIndex(concat_table->index(0, 0));

	edit->setClearButtonEnabled(true);
	edit->setPlaceholderText("Find anything");
	edit->addAction(QIcon("data/icons/object_editor/search.png"), QLineEdit::LeadingPosition);
	edit->installEventFilter(this);

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget(edit);
	layout->addWidget(list);
	layout->setSpacing(3);

	setLayout(layout);
	show();

	edit->installEventFilter(this);

	connect(edit, &QLineEdit::textEdited, ability_filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(edit, &QLineEdit::textEdited, item_filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(edit, &QLineEdit::textEdited, units_filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(edit, &QLineEdit::textEdited, doodad_filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(edit, &QLineEdit::textEdited, destructable_filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(edit, &QLineEdit::textEdited, upgrade_filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(edit, &QLineEdit::textEdited, buff_filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(edit, &QLineEdit::textEdited, [&] {
		list->setCurrentIndex(concat_table->index(0, 0));
	});

	connect(list, &QListView::activated, [=](const QModelIndex& index) {
		std::string id;
		const auto model = concat_table->mapToSource(index).model();
		TableModel* table_model = nullptr;
		if (model == units_filter_model) {
			const int row = units_filter_model->mapToSource(concat_table->mapToSource(index)).row();
			id = units_slk.index_to_row.at(row);
			table_model = units_table;
		} else if (model == destructable_filter_model) {
			const int row = destructable_filter_model->mapToSource(concat_table->mapToSource(index)).row();
			id = destructibles_slk.index_to_row.at(row);
			table_model = destructibles_table;
		} else if (model == doodad_filter_model) {
			const int row = doodad_filter_model->mapToSource(concat_table->mapToSource(index)).row();
			id = doodads_slk.index_to_row.at(row);
			table_model = doodads_table;
		} else if (model == item_filter_model) {
			const int row = item_filter_model->mapToSource(concat_table->mapToSource(index)).row();
			id = items_slk.index_to_row.at(row);
			table_model = items_table;
		} else if (model == ability_filter_model) {
			const int row = ability_filter_model->mapToSource(concat_table->mapToSource(index)).row();
			id = abilities_slk.index_to_row.at(row);
			table_model = abilities_table;
		} else if (model == buff_filter_model) {
			const int row = buff_filter_model->mapToSource(concat_table->mapToSource(index)).row();
			id = buff_slk.index_to_row.at(row);
			table_model = buff_table;
		} else if (model == upgrade_filter_model) {
			const int row = upgrade_filter_model->mapToSource(concat_table->mapToSource(index)).row();
			id = upgrade_slk.index_to_row.at(row);
			table_model = upgrade_table;
		}

		bool created;
		auto* object_editor = window_handler.create_or_raise<ObjectEditor>(nullptr, created);
		object_editor->open_by_id(table_model, id, index.data(Qt::DisplayRole).toString(), index.data(Qt::DecorationRole).value<QIcon>());
	});
}

bool GlobalSearchWidget::eventFilter(QObject *object, QEvent *event) {
	if (object == edit && event->type() == QEvent::KeyPress) {
		const QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(event);

		if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
			QCoreApplication::sendEvent(list, event);
			return true;
		}
	}
	return false;
}