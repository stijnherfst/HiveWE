#pragma once

#include <QSortFilterProxyModel>

#ifdef __linux__
#include "std_compat.h"
#endif
#ifndef __linux__
import std;
#endif
import Globals;
#include "models/list/base_list_model.h"
class AbilityListModel: public BaseListModel {

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

class AbilityListFilter: public QSortFilterProxyModel {

	[[nodiscard]]
	bool filterAcceptsRow(const int sourceRow, [[maybe_unused]] const QModelIndex& sourceParent) const override {
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
