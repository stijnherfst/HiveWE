#include "doodad_list_model.h"

#include <ankerl/unordered_dense.h>
#include "globals.h"

DoodadListModel::DoodadListModel(QObject* parent) : QIdentityProxyModel(parent) {
	for (auto&& [key, value] : world_edit_data.section("DoodadCategories")) {
		const std::string tileset_key = value.front();
		icons[key.front()] = resource_manager.load<QIconResource>(value[1]);
	}
}

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

	return sourceModel()->index(proxyIndex.row(), doodads_slk.column_headers.at("name"));
}

QVariant DoodadListModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return {};
	}

	switch (role) {
		case Qt::DisplayRole:
			return sourceModel()->data(mapToSource(index), role).toString();
		case Qt::DecorationRole: {
			char category = doodads_slk.data("category", index.row()).front();
			if (icons.contains(category)) {
				return icons.at(category)->icon;
			} else {
				return {};
			}
		}
		case Qt::SizeHintRole:
			return QSize(0, 16);
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
	return sourceModel()->rowCount();
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

	if (!filterRegularExpression().pattern().isEmpty()) {
		return sourceModel()->data(index0).toString().contains(filterRegularExpression());
	} else {
		const std::string tilesets = doodads_slk.data("tilesets", sourceRow);
		return QString::fromStdString(doodads_slk.data("category", sourceRow)) == filterCategory && (tilesets.find('*') != std::string::npos || tilesets.find(filterTileset) != std::string::npos || filterTileset == '*');
	}
}

bool DoodadListFilter::lessThan(const QModelIndex& left, const QModelIndex& right) const {
	return doodads_slk.data("name", left.row()) < doodads_slk.data("name", right.row());
}

void DoodadListFilter::setFilterCategory(QString category) {
	filterCategory = category;
	invalidateFilter();
}

void DoodadListFilter::setFilterTileset(char tileset) {
	filterTileset = tileset;
	invalidateFilter();
}