module;

#include <QIdentityProxyModel>
#include <QSortFilterProxyModel>

export module UnitListModel;

import Globals;

export class UnitListModel: public QIdentityProxyModel {
	Q_OBJECT

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

		return sourceModel()->index(proxyIndex.row(), units_slk.column_headers.at("name"));
	}

	[[nodiscard]]
	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid()) {
			return {};
		}

		switch (role) {
			case Qt::DisplayRole:
				return mapToSource(index).data(role).toString() + " "
					+ sourceModel()->index(index.row(), units_slk.column_headers.at("editorsuffix")).data(role).toString();
			case Qt::DecorationRole:
				return sourceModel()->index(index.row(), units_slk.column_headers.at("art")).data(role);
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
	int rowCount(const QModelIndex& parent) const override {
		return units_slk.rows();
	}

	[[nodiscard]]
	int columnCount(const QModelIndex& parent) const override {
		return 1;
	}

	[[nodiscard]]
	QModelIndex index(int row, int column, const QModelIndex& parent) const override {
		return createIndex(row, column);
	}

	[[nodiscard]]
	QModelIndex parent(const QModelIndex& child) const override {
		return QModelIndex();
	}

	using QIdentityProxyModel::QIdentityProxyModel;
};

export class UnitListFilter: public QSortFilterProxyModel {
	Q_OBJECT

	[[nodiscard]]
	bool filterAcceptsRow(const int sourceRow, const QModelIndex& sourceParent) const override {
		if (QString::fromStdString(units_slk.index_to_row.at(sourceRow)).contains(filterRegularExpression())) {
			return true;
		}

		if (!filterRegularExpression().pattern().isEmpty()) {
			const QModelIndex source_index = sourceModel()->index(sourceRow, 0);
			return source_index.data().toString().contains(filterRegularExpression());
		} else {
			return QString::fromStdString(units_slk.data(units_slk.column_headers.at("race"), sourceRow)) == filterRace;
		}
	}

	[[nodiscard]]
	bool lessThan(const QModelIndex& left, const QModelIndex& right) const override {
		QString leftIndex = "0";
		{
			const bool isHostile = units_slk.data("hostilepal", left.row()) == "1";
			const bool isBuilding = units_slk.data("isbldg", left.row()) == "1";
			const bool isHero = isupper(units_slk.index_to_row.at(left.row()).front());
			const bool isSpecial = units_slk.data("special", left.row()) == "1";

			if (isSpecial) {
				leftIndex = "3";
			} else if (isBuilding) {
				leftIndex = "1";
			} else if (isHero) {
				leftIndex = "2";
			}
			leftIndex += QString::fromStdString(units_slk.data("name", left.row()));
		}

		QString rightIndex = "0";
		{
			const bool isHostile = units_slk.data("hostilepal", right.row()) == "1";
			const bool isBuilding = units_slk.data("isbldg", right.row()) == "1";
			const bool isHero = isupper(units_slk.index_to_row.at(right.row()).front());
			const bool isSpecial = units_slk.data("special", right.row()) == "1";

			if (isSpecial) {
				rightIndex = "3";
			} else if (isBuilding) {
				rightIndex = "1";
			} else if (isHero) {
				rightIndex = "2";
			}
			rightIndex += QString::fromStdString(units_slk.data("name", right.row()));
		}

		return leftIndex < rightIndex;
	}

	QString filterRace = "human";

  public:
	using QSortFilterProxyModel::QSortFilterProxyModel;

  public slots:

	void setFilterRace(const QString& race) {
		filterRace = race;
		invalidateFilter();
	}
};

#include "unit_list_model.moc"
