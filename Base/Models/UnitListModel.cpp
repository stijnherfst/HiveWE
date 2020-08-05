#include "UnitListModel.h"

#include "HiveWE.h"

QModelIndex UnitListModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}

	return createIndex(sourceIndex.row(), 0);
}

QModelIndex UnitListModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	return sourceModel()->index(proxyIndex.row(), units_slk.column_headers.at("name"));
}

QVariant UnitListModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return {};
	}

	switch (role) {
		case Qt::DisplayRole:
			return sourceModel()->data(mapToSource(index), role).toString() + " " + 
				sourceModel()->data(sourceModel()->index(index.row(), units_slk.column_headers.at("editorsuffix")), role).toString();
		case Qt::DecorationRole:
			return sourceModel()->data(sourceModel()->index(index.row(), units_slk.column_headers.at("art")), role);
		case Qt::SizeHintRole:
			return QSize(0, 24);
		default:
			return sourceModel()->data(mapToSource(index), role);
	}
}

Qt::ItemFlags UnitListModel::flags(const QModelIndex& index) const {
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int UnitListModel::rowCount(const QModelIndex& parent) const {
	return units_slk.rows();
}

int UnitListModel::columnCount(const QModelIndex& parent) const {
	return 1;
}

QModelIndex UnitListModel::index(int row, int column, const QModelIndex& parent) const {
	return createIndex(row, column);
}

QModelIndex UnitListModel::parent(const QModelIndex& child) const {
	return QModelIndex();
}

void UnitListModel::setSourceModel(QAbstractItemModel* sourceModel) {
	beginResetModel();
	QAbstractProxyModel::setSourceModel(sourceModel);
	endResetModel();

	connect(sourceModel, &QAbstractItemModel::dataChanged, this, [&](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) { 
		emit dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), roles); 
	});
}

bool UnitListFilter::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
	QModelIndex index0 = sourceModel()->index(sourceRow, 0);

	if (!filterRegExp().isEmpty()) {
		return sourceModel()->data(index0).toString().contains(filterRegExp());
	} else {
		return QString::fromStdString(units_slk.data(units_slk.column_headers.at("race"), sourceRow)) == filterRace;
	}
}

bool UnitListFilter::lessThan(const QModelIndex& left, const QModelIndex& right) const {

	QString leftIndex = "0";
	{
		bool isHostile = units_slk.data("hostilepal", left.row()) == "1";
		bool isBuilding = units_slk.data("isbldg", left.row()) == "1";
		bool isHero = isupper(units_slk.index_to_row.at(left.row()).front());
		bool isSpecial = units_slk.data("special", left.row()) == "1";

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
		bool isHostile = units_slk.data("hostilepal", right.row()) == "1";
		bool isBuilding = units_slk.data("isbldg", right.row()) == "1";
		bool isHero = isupper(units_slk.index_to_row.at(right.row()).front());
		bool isSpecial = units_slk.data("special", right.row()) == "1";

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

void UnitListFilter::setFilterRace(QString race) {
	filterRace = race;
	invalidateFilter();
}