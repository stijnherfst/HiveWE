#include "DestructableListModel.h"

#include "HiveWE.h"

DestructableListModel::DestructableListModel(QObject* parent) : QAbstractProxyModel(parent) {
	for (auto&& [key, value] : world_edit_data.section("DestructibleCategories")) {
		const std::string tileset_key = value.front();
		icons[key.front()] = resource_manager.load<QIconResource>(value[1]);
	}
}

QModelIndex DestructableListModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}

	return createIndex(sourceIndex.row(), 0);
}

QModelIndex DestructableListModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	return sourceModel()->index(proxyIndex.row(), destructibles_slk.column_headers.at("name"));
}

QVariant DestructableListModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return {};
	}

	switch (role) {
		case Qt::DisplayRole:
			return sourceModel()->data(mapToSource(index), role).toString() + " " + QString::fromStdString(destructibles_slk.data("editorsuffix", index.row()));
		case Qt::DecorationRole: {
			char category = destructibles_slk.data("category", index.row()).front();
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

Qt::ItemFlags DestructableListModel::flags(const QModelIndex& index) const {
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int DestructableListModel::rowCount(const QModelIndex& parent) const {
	return sourceModel()->rowCount();
}

int DestructableListModel::columnCount(const QModelIndex& parent) const {
	return 1;
}

QModelIndex DestructableListModel::index(int row, int column, const QModelIndex& parent) const {
	return createIndex(row, column);
}

QModelIndex DestructableListModel::parent(const QModelIndex& child) const {
	return QModelIndex();
}

void DestructableListModel::setSourceModel(QAbstractItemModel* sourceModel) {
	beginResetModel();
	QAbstractProxyModel::setSourceModel(sourceModel);
	endResetModel();

	connect(sourceModel, &QAbstractItemModel::dataChanged, this, [&](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
		emit dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), roles);
	});
}

bool DestructableListFilter::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
	QModelIndex index0 = sourceModel()->index(sourceRow, 0);

	if (!filterRegExp().isEmpty()) {
		return sourceModel()->data(index0).toString().contains(filterRegExp()) ;
	} else {
		const std::string tilesets = destructibles_slk.data("tilesets", sourceRow);
		return QString::fromStdString(destructibles_slk.data("category", sourceRow)) == filterCategory && (tilesets.find('*') != std::string::npos || tilesets.find(filterTileset) != std::string::npos || filterTileset == '*');;
	}
}

bool DestructableListFilter::lessThan(const QModelIndex& left, const QModelIndex& right) const {
	return destructibles_slk.data("name", left.row()) < destructibles_slk.data("name", right.row());
}

void DestructableListFilter::setFilterCategory(QString category) {
	filterCategory = category;
	invalidateFilter();
}

void DestructableListFilter::setFilterTileset(char tileset) {
	filterTileset = tileset;
	invalidateFilter();
}