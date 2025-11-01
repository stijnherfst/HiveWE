#include "global_search.h"

#include "object_editor.h"

#include <QLayout>
#include <QFrame>
#include <QDialog>
#include <QSortFilterProxyModel>
#include <ui_object_editor.h>

import WindowHandler;
import Globals;
import TableModel;

GlobalSearchWidget::GlobalSearchWidget(QWidget* parent) : QDialog(parent) {
	setWindowFlag(Qt::FramelessWindowHint, true);
	setAttribute(Qt::WA_DeleteOnClose);

	resize(400, 700);

	unit_list_model = new UnitListModel(this);
	unit_list_model->setSourceModel(units_table);

	units_filter_model = new UnitListFilter(this);
	units_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	units_filter_model->setSourceModel(unit_list_model);
	units_filter_model->sort(0, Qt::AscendingOrder);

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
	concat_table->addSourceModel(units_filter_model);
	concat_table->addSourceModel(destructable_filter_model);
	concat_table->addSourceModel(doodad_filter_model);

	list = new QListView;
	list->setIconSize(QSize(32, 32));
	list->setModel(concat_table);
	list->setUniformItemSizes(true);

	edit->setClearButtonEnabled(true);
	edit->setPlaceholderText("Find anything");
	edit->addAction(QIcon("data/icons/object_editor/search.png"), QLineEdit::LeadingPosition);

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget(edit);
	layout->addWidget(list);
	layout->setSpacing(3);

	setLayout(layout);
	show();

	connect(edit, &QLineEdit::textEdited, units_filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(edit, &QLineEdit::textEdited, doodad_filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(edit, &QLineEdit::textEdited, destructable_filter_model, &QSortFilterProxyModel::setFilterFixedString);

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
		}

		bool created;
		auto* object_editor = window_handler.create_or_raise<ObjectEditor>(nullptr, created);
		object_editor->open_by_id(table_model, id, index.data(Qt::DisplayRole).toString(), index.data(Qt::DecorationRole).value<QIcon>());
	});
}
