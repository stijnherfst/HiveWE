#include "gameplay_constants_editor.h"

import std;
import MapGlobal;
import GameplayConstants;
import TableModel;

#include "single_model.h"

#include <QHeaderView>
#include <QTableView>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QVBoxLayout>

GameplayConstantsEditor::GameplayConstantsEditor(QWidget* parent) : QDialog(parent) {
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle("Gameplay Constants");
	resize(1000, 600);

	auto* layout = new QVBoxLayout(this);
	search = new QLineEdit(this);
	search->setPlaceholderText("Search...");
	layout->addWidget(search);

	TableModel* table = new TableModel(&map->gameplay_constants.data, &map->gameplay_constants.metadata, &map->trigger_strings, this);
	SingleModel* single_model = new SingleModel(table, this);
	single_model->setID(constants_row_key);

	QSortFilterProxyModel* proxy_model = new QSortFilterProxyModel(this);
	proxy_model->setSourceModel(single_model);
	proxy_model->setFilterKeyColumn(-1); // All columns
	proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);

	TableDelegate* delegate = new TableDelegate(single_model);

	QTableView* view = new QTableView;
	layout->addWidget(view);
	view->setVerticalHeader(new AlterHeader(Qt::Vertical, view));
	view->setModel(proxy_model);
	view->setItemDelegate(delegate);
	view->horizontalHeader()->hide();
	view->setAlternatingRowColors(true);
	view->setIconSize({24, 24});
	layout->addWidget(view);

	connect(search, &QLineEdit::textChanged, proxy_model, &QSortFilterProxyModel::setFilterFixedString);

	show();

	// So the columns don't resize on search
	QTimer::singleShot(0, [view, proxy_model]() {
		view->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
		const int vertical_width = view->verticalHeader()->width();
		view->verticalHeader()->setFixedWidth(vertical_width);

		view->resizeColumnsToContents();
		for (int i = 0; i < proxy_model->columnCount(); ++i) {
			const int horizontal_width = view->columnWidth(i);

			view->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Fixed);
			view->setColumnWidth(i, horizontal_width);
		}
	});
}
