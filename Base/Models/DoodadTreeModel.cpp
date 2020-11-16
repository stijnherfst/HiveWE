#include "DoodadTreeModel.h"

DoodadTreeModel::DoodadTreeModel(QObject* parent) : BaseTreeModel(parent) {
	slk = &doodads_slk;

	for (const auto& [key, value] : world_edit_data.section("DoodadCategories")) {
		categories[key.front()].name = value[0];
		categories[key.front()].icon = resource_manager.load<QIconResource>(value[1]);
		categories[key.front()].item = new BaseTreeItem(rootItem);
		categories[key.front()].item->baseCategory = true;
		rowToCategory.push_back(key.front());
	}

	for (int i = 0; i < doodads_slk.rows(); i++) {
		const std::string& id = doodads_slk.index_to_row.at(i);
		BaseTreeItem* item = new BaseTreeItem(getFolderParent(id));
		item->id = id;
	}
}

BaseTreeItem* DoodadTreeModel::getFolderParent(const std::string& id) const {
	std::string category = doodads_slk.data("category", id);

	return categories.at(category.front()).item;
}

QVariant DoodadTreeModel::data(const QModelIndex& index, int role) const {
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
				return QAbstractProxyModel::data(index, role).toString();
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

			if (doodads_slk.shadow_data.contains(item->id)) {
				return QColor("violet");
			} else {
				return {};
			}
		default:
			return {};
	}
}