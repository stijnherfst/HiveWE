#include "ItemTreeModel.h"

ItemTreeModel::ItemTreeModel(QObject* parent) : BaseTreeModel(parent) {
	slk = &items_slk;

	for (const auto& [key, value] : unit_editor_data.section("itemClass")) {
		if (key == "Sort" || key == "NumValues") {
			continue;
		}

		categories[value[0]].name = value[1];
		categories[value[0]].item = new BaseTreeItem(rootItem);
		categories[value[0]].item->baseCategory = true;
		rowToCategory.push_back(value[0]);
	}

	for (int i = 0; i < items_slk.rows(); i++) {
		const std::string& id = items_slk.index_to_row.at(i);

		BaseTreeItem* parent_item = getFolderParent(id);
		if (!parent_item) {
			continue;
		}
		BaseTreeItem* item = new BaseTreeItem(parent_item);
		item->id = id;
	}
}

BaseTreeItem* ItemTreeModel::getFolderParent(const std::string& id) const {
	std::string itemClass = items_slk.data("class", id);
	if (itemClass.empty()) {
		std::cout << "Empty class for " << id << " in items\n";
		return nullptr;
	}

	return categories.at(itemClass).item;
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
			if (item->baseCategory || item->subCategory) {
				return folderIcon;
			}
			return sourceModel()->data(sourceModel()->index(items_slk.row_headers.at(item->id), items_slk.column_headers.at("art")), role);
		case Qt::TextColorRole:
			if (item->baseCategory || item->subCategory) {
				return {};
			}

			if (items_slk.shadow_data.contains(item->id)) {
				return QColor("violet");
			} else {
				return {};
			}
		default:
			return {};
	}
}