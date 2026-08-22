#pragma once

#include <QIdentityProxyModel>
#include <QSortFilterProxyModel>
#include <QSize>
import Globals;
import SLK;
class BaseListModel: public QIdentityProxyModel {

  public:
	[[nodiscard]]
	QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override {
		if (!sourceIndex.isValid()) {
			return {};
		}

		return createIndex(sourceIndex.row(), 0);
	}

	[[nodiscard]]
	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override {
		if (!proxyIndex.isValid()) {
			return {};
		}

		return sourceModel()->index(proxyIndex.row(), slk.column_headers.at("name"));
	}

	[[nodiscard]]
	QVariant data(const QModelIndex& index, const int role) const override {
		if (!index.isValid()) {
			return {};
		}

		switch (role) {
			case Qt::DisplayRole:
				return mapToSource(index).data(role).toString();
			case Qt::DecorationRole:
				return sourceModel()->index(index.row(), items_slk.column_headers.at("art")).data(role);
			default:
				return mapToSource(index).data(role);
		}
	}

	[[nodiscard]]
	Qt::ItemFlags flags(const QModelIndex& index) const override {
		if (!index.isValid()) {
			return Qt::NoItemFlags;
		}

		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	[[nodiscard]]
	int rowCount(const QModelIndex&) const override {
		return sourceModel()->rowCount();
	}

	[[nodiscard]]
	int columnCount(const QModelIndex&) const override {
		return 1;
	}

	[[nodiscard]]
	QModelIndex index(const int row, const int column, const QModelIndex&) const override {
		return createIndex(row, column);
	}

	[[nodiscard]]
	QModelIndex parent(const QModelIndex&) const override {
		return QModelIndex();
	}

	explicit BaseListModel(const slk::SLK& slk, QObject* parent = nullptr) : QIdentityProxyModel(parent), slk(slk) {}

	const slk::SLK& slk;
};
