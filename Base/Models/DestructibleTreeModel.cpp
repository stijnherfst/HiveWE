#include "DestructibleTreeModel.h"

DestructibleTreeModel::DestructibleTreeModel(QObject* parent) : BaseTreeModel(parent) {
	for (const auto& [key, value] : world_edit_data.section("DestructibleCategories")) {
		categories[key.front()].name = value[0];
		categories[key.front()].icon = resource_manager.load<QIconResource>(value[1]);
		categories[key.front()].item = new BaseTreeItem(rootItem);
		categories[key.front()].item->baseCategory = true;
		rowToCategory.push_back(key.front());
	}

	for (int i = 0; i < destructibles_slk.rows(); i++) {
		std::string category = destructibles_slk.data("category", i);
		BaseTreeItem* item = new BaseTreeItem(categories[category.front()].item);
		item->tableRow = i;
	}
}

QModelIndex DestructibleTreeModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}

	std::string category = destructibles_slk.data("category", sourceIndex.row());

	auto& items = categories.at(category.front()).item->children;
	for (int i = 0; i < items.size(); i++) {
		BaseTreeItem* item = items[i];
		if (item->tableRow == sourceIndex.row()) {
			return createIndex(i, 0, item);
		}
	}

	return {};
}

QModelIndex DestructibleTreeModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	BaseTreeItem* item = static_cast<BaseTreeItem*>(proxyIndex.internalPointer());
	if (!item->baseCategory) {
		return createIndex(item->tableRow, destructibles_slk.column_headers.at("name"), item);
	}
	return {};
}

QVariant DestructibleTreeModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return {};
	}

	BaseTreeItem* item = static_cast<BaseTreeItem*>(index.internalPointer());

	switch (role) {
		case Qt::EditRole:
		case Qt::DisplayRole:
			if (item->baseCategory) {
				return QString::fromStdString(categories.at(rowToCategory[index.row()]).name);
			} else {
				return QAbstractProxyModel::data(index, role).toString() + " " + QString::fromStdString(destructibles_slk.data("editorsuffix", item->tableRow));
			}
		case Qt::DecorationRole:
			if (item->baseCategory || item->subCategory) {
				return folderIcon;
			}

			return categories.at(rowToCategory[index.parent().row()]).icon->icon;
		case Qt::TextColorRole:
			if (item->baseCategory || item->subCategory) {
				return {};
			}

			if (destructibles_slk.shadow_data.contains(destructibles_slk.index_to_row.at(item->tableRow))) {
				return QColor("violet");
			} else {
				return {};
			}
		default:
			return {};
	}
}