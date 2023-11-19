module;

#include <QListView>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QSortFilterProxyModel>

#include "globals.h"

export module UnitSelector;

import UnitListModel;
import TableModel;

export class UnitSelector : public QWidget {
	Q_OBJECT

  public:
	UnitSelector(QWidget* parent = nullptr)
		: QWidget(parent) {
		list_model = new UnitListModel(this);
		list_model->setSourceModel(units_table);

		filter_model = new UnitListFilter(this);
		filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
		filter_model->setSourceModel(list_model);
		filter_model->sort(0, Qt::AscendingOrder);

		race = new QComboBox(this);
		search = new QLineEdit(this);
		units = new QListView(this);

		search->setPlaceholderText("Search");

		units->setModel(filter_model);

		QVBoxLayout* layout = new QVBoxLayout;
		layout->setContentsMargins(0, 0, 0, 0);
		layout->addWidget(race);
		layout->addWidget(search);
		layout->addWidget(units);
		setLayout(layout);

		for (const auto& [key, value] : unit_editor_data.section("unitRace")) {
			if (key == "Sort" || key == "NumValues") {
				continue;
			}
			race->addItem(QString::fromStdString(value[1]), QString::fromStdString(value[0]));
		}

		connect(race, QOverload<int>::of(&QComboBox::currentIndexChanged), [&]() {
			filter_model->setFilterRace(race->currentData().toString());
		});

		connect(search, &QLineEdit::textEdited, filter_model, &QSortFilterProxyModel::setFilterFixedString);
		connect(search, &QLineEdit::returnPressed, [&]() {
			units->setCurrentIndex(units->model()->index(0, 0));
			units->setFocus();
		});

		connect(units, &QListView::clicked, [&](const QModelIndex& index) {
			const int row = filter_model->mapToSource(index).row();
			emit unitSelected(units_slk.index_to_row.at(row));
		});
		connect(units, &QListView::activated, [&](const QModelIndex& index) {
			const int row = filter_model->mapToSource(index).row();
			emit unitSelected(units_slk.index_to_row.at(row));
		});
	}

	UnitListModel* list_model;
	UnitListFilter* filter_model;

	QComboBox* race;
	QLineEdit* search;
	QListView* units;

  public slots:
	void forceSelection() {
		if (units->selectionModel()->selectedRows().isEmpty()) {
			return;
		}
		QModelIndex index = units->selectionModel()->selectedRows().front();
		if (!index.isValid()) {
			return;
		}
		const int row = filter_model->mapToSource(index).row();
		emit unitSelected(units_slk.index_to_row.at(row));
	}

  signals:
	void unitSelected(std::string id);
};

#include "unit_selector.moc"