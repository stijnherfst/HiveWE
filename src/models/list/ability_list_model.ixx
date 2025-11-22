module;

#include <QSortFilterProxyModel>

export module AbilityListModel;

import std;
import Globals;
import BaseListModel;

export class AbilityListModel: public BaseListModel {
	Q_OBJECT

  public:
	explicit AbilityListModel(QObject* parent = nullptr) : BaseListModel(abilities_slk, parent) {}

	[[nodiscard]]
	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override {
		if (!proxyIndex.isValid()) {
			return {};
		}

		return sourceModel()->index(proxyIndex.row(), abilities_slk.column_headers.at("name"));
	}

	[[nodiscard]]
	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid()) {
			return {};
		}

		switch (role) {
			case Qt::DisplayRole:
				return mapToSource(index).data(role).toString() + " " + QString::fromUtf8(abilities_slk.data<std::string_view>("editorsuffix", index.row()));
			case Qt::UserRole:
				return QString::fromStdString("abilities/" + abilities_slk.data("race", index.row()) + "/" + abilities_slk.index_to_row.at(index.row()));
			case Qt::DecorationRole:
				return sourceModel()->index(index.row(), abilities_slk.column_headers.at("art")).data(role);

			default:
				return BaseListModel::data(index, role);
		}
	}
};

export class AbilityListFilter: public QSortFilterProxyModel {
	Q_OBJECT

	[[nodiscard]]
	bool filterAcceptsRow(const int sourceRow, const QModelIndex& sourceParent) const override {
		if (!filterRegularExpression().pattern().isEmpty()) {
			if (QString::fromStdString(abilities_slk.index_to_row.at(sourceRow)).contains(filterRegularExpression())) {
				return true;
			}

			const QModelIndex source_index = sourceModel()->index(sourceRow, 0);
			return source_index.data().toString().contains(filterRegularExpression());
		}
		return true;
	}

  public:
	using QSortFilterProxyModel::QSortFilterProxyModel;
};

#include "ability_list_model.moc"
