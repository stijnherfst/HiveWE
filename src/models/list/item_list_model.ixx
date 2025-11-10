module;

#include <QSortFilterProxyModel>

export module ItemListModel;

import Globals;
import BaseListModel;

export class ItemListModel: public BaseListModel {
	Q_OBJECT

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
			case Qt::DecorationRole:
				return sourceModel()->index(index.row(), items_slk.column_headers.at("art")).data(role);
			default:
				return BaseListModel::data(index, role);
		}
	}
};

export class ItemListFilter: public QSortFilterProxyModel {
	Q_OBJECT

	[[nodiscard]]
	bool filterAcceptsRow(const int sourceRow, const QModelIndex& sourceParent) const override {
		if (QString::fromStdString(items_slk.index_to_row.at(sourceRow)).contains(filterRegularExpression())) {
			return true;
		}

		if (!filterRegularExpression().pattern().isEmpty()) {
			const QModelIndex source_index = sourceModel()->index(sourceRow, 0);
			return source_index.data().toString().contains(filterRegularExpression());
		}
		return true;
	}

public:
	using QSortFilterProxyModel::QSortFilterProxyModel;
};

#include "item_list_model.moc"
