module;

#include <QSortFilterProxyModel>

export module UpgradeListModel;

import BaseListModel;
import Globals;

export class UpgradeListModel: public BaseListModel {
	Q_OBJECT

  public:
	explicit UpgradeListModel(QObject* parent = nullptr) : BaseListModel(upgrade_slk, parent) {}

	[[nodiscard]]
	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override {
		if (!proxyIndex.isValid()) {
			return {};
		}

		return sourceModel()->index(proxyIndex.row(), upgrade_slk.column_headers.at("name1"));
	}

	[[nodiscard]]
	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid()) {
			return {};
		}

		switch (role) {
			case Qt::DisplayRole:
				return mapToSource(index).data(role).toString() + " " + QString::fromStdString(upgrade_slk.data("editorsuffix", index.row()));
			case Qt::UserRole:
				return QString::fromStdString("upgrades/" + upgrade_slk.data("race", index.row()) + "/" + upgrade_slk.index_to_row.at(index.row()));
			case Qt::DecorationRole:
				return sourceModel()->index(index.row(), upgrade_slk.column_headers.at("art1")).data(role);
			default:
				return BaseListModel::data(index, role);
		}
	}
};

export class UpgradeListFilter: public QSortFilterProxyModel {
	Q_OBJECT

	[[nodiscard]]
	bool filterAcceptsRow(const int sourceRow, const QModelIndex& sourceParent) const override {
		if (QString::fromStdString(upgrade_slk.index_to_row.at(sourceRow)).contains(filterRegularExpression())) {
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

#include "upgrade_list_model.moc"
