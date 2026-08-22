#pragma once

#include <QSortFilterProxyModel>
import Globals;
#include "models/list/base_list_model.h"
class ItemListModel: public BaseListModel {

  public:
	explicit ItemListModel(QObject* parent = nullptr) : BaseListModel(items_slk, parent) {}

	[[nodiscard]]
	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override {
		if (!proxyIndex.isValid()) {
			return {};
		}

		return sourceModel()->index(proxyIndex.row(), items_slk.column_headers.at("name"));
	}

	[[nodiscard]]
	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid()) {
			return {};
		}

		switch (role) {
			case Qt::DisplayRole:
				return mapToSource(index).data(role).toString();
			case Qt::UserRole:
				return QString::fromStdString("items/" + items_slk.data("class", index.row()) + "/" + items_slk.index_to_row.at(index.row()));
			case Qt::DecorationRole:
				return sourceModel()->index(index.row(), items_slk.column_headers.at("art")).data(role);
			default:
				return BaseListModel::data(index, role);
		}
	}
};

class ItemListFilter: public QSortFilterProxyModel {

	[[nodiscard]]
	bool filterAcceptsRow(const int sourceRow, [[maybe_unused]] const QModelIndex& sourceParent) const override {
		if (!filterRegularExpression().pattern().isEmpty()) {
			if (QString::fromStdString(items_slk.index_to_row.at(sourceRow)).contains(filterRegularExpression())) {
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
