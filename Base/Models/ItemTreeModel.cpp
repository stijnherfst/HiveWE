#include "ItemTreeModel.h"

ItemTreeModel::ItemTreeModel(QObject* parent) : BaseTreeModel(parent) {
	for (const auto& [key, value] : unit_editor_data.section("itemClass")) {
		if (key == "Sort" || key == "NumValues") {
			continue;
		}

		categories[value[0]].name = value[1];
		categories[value[0]].item = new BaseTreeItem(rootItem);
		categories[value[0]].item->baseCategory = true;
		rowToCategory.push_back(value[0]);
	}

	// Start at 1 since the first row are column headers
	for (int i = 1; i < items_slk.rows; i++) {
		std::string itemClass = items_slk.data("class", i);
		if (itemClass.empty()) {
			std::cout << "Empty class for " << i << " in items\n";
			continue;
		}

		BaseTreeItem* item = new BaseTreeItem(categories[itemClass].item);
		item->tableRow = i;
	}
}

QModelIndex ItemTreeModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}

	std::string itemClass = items_slk.data("class", sourceIndex.row());

	auto items = categories.at(itemClass).item->children;
	for (int i = 0; i < items.size(); i++) {
		BaseTreeItem* item = items[i];
		if (item->tableRow == sourceIndex.row()) {
			return createIndex(i, 0, item);
		}
	}

	return {};
}

QModelIndex ItemTreeModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	BaseTreeItem* item = static_cast<BaseTreeItem*>(proxyIndex.internalPointer());
	if (!item->baseCategory) {
		return createIndex(item->tableRow, items_slk.header_to_column.at("name"), item);
	}
	return {};
}

QVariant ItemTreeModel::data(const QModelIndex& index, int role) const {
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
				return QAbstractProxyModel::data(index, role);
			}
		case Qt::DecorationRole:
			if (item->tableRow < 0) {
				return folderIcon;
			}
			return sourceModel()->data(sourceModel()->index(item->tableRow, items_slk.header_to_column.at("art")), role);
		default:
			return {};
	}
}