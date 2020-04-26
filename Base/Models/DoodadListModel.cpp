#include "DoodadListModel.h"

#include "HiveWE.h"

QModelIndex DoodadListModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}

	return createIndex(sourceIndex.row(), 0);
}

QModelIndex DoodadListModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	return sourceModel()->index(proxyIndex.row(), doodads_slk.header_to_column.at("name"));
}

QVariant DoodadListModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return {};
	}

	switch (role) {
		case Qt::DisplayRole:

			//bool is_doodad = world_edit_data.key_exists("DoodadCategories", selected_category);
			//slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

			return sourceModel()->data(mapToSource(index), role).toString() + " " + QString::fromStdString(doodads_slk.data("doodid", index.row()));
		case Qt::DecorationRole:
			return{};
			//return sourceModel()->data(sourceModel()->index(index.row(), units_slk.header_to_column.at("art")), role);
		case Qt::SizeHintRole:
			return QSize(0, 24);
		default:
			return sourceModel()->data(mapToSource(index), role);
	}
}

Qt::ItemFlags DoodadListModel::flags(const QModelIndex& index) const {
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int DoodadListModel::rowCount(const QModelIndex& parent) const {
	return doodads_slk.rows;
}

int DoodadListModel::columnCount(const QModelIndex& parent) const {
	return 1;
}

QModelIndex DoodadListModel::index(int row, int column, const QModelIndex& parent) const {
	return createIndex(row, column);
}

QModelIndex DoodadListModel::parent(const QModelIndex& child) const {
	return QModelIndex();
}

bool DoodadListFilter::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
	QModelIndex index0 = sourceModel()->index(sourceRow, 0);

	if (!filterRegExp().isEmpty()) {
		return sourceModel()->data(index0).toString().contains(filterRegExp());
	} else {
		return QString::fromStdString(doodads_slk.data(doodads_slk.header_to_column.at("race"), sourceRow)) == filterRace;
	}
}

bool DoodadListFilter::lessThan(const QModelIndex& left, const QModelIndex& right) const {

	QString leftIndex = "0";
	{
		bool isHostile = doodads_slk.data("hostilepal", left.row()) == "1";
		bool isBuilding = doodads_slk.data("isbldg", left.row()) == "1";
		QString unitID = QString::fromStdString(doodads_slk.data("unitid", left.row()));
		bool isHero = unitID.front().isUpper();
		bool isSpecial = doodads_slk.data("special", left.row()) == "1";

		if (isSpecial) {
			leftIndex = "3";
		} else if (isBuilding) {
			leftIndex = "1";
		} else if (isHero) {
			leftIndex = "2";
		}
		leftIndex += QString::fromStdString(doodads_slk.data("name", left.row()));
	}

	QString rightIndex = "0";
	{
		bool isHostile = doodads_slk.data("hostilepal", right.row()) == "1";
		bool isBuilding = doodads_slk.data("isbldg", right.row()) == "1";
		QString unitID = QString::fromStdString(doodads_slk.data("unitid", right.row()));
		bool isHero = unitID.front().isUpper();
		bool isSpecial = doodads_slk.data("special", right.row()) == "1";

		if (isSpecial) {
			rightIndex = "3";
		} else if (isBuilding) {
			rightIndex = "1";
		} else if (isHero) {
			rightIndex = "2";
		}
		rightIndex += QString::fromStdString(doodads_slk.data("name", right.row()));
	}

	return leftIndex < rightIndex;
}

void DoodadListFilter::setFilterRace(QString race) {
	filterRace = race;
	invalidateFilter();
}