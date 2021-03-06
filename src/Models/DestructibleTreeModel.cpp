#include "DestructibleTreeModel.h"

DestructibleTreeModel::DestructibleTreeModel(QObject* parent) : BaseTreeModel(parent) {
	slk = &destructibles_slk;

	for (const auto& [key, value] : world_edit_data.section("DestructibleCategories")) {
		categories[key.front()].name = value[0];
		categories[key.front()].icon = resource_manager.load<QIconResource>(value[1]);
		categories[key.front()].item = new BaseTreeItem(rootItem);
		categories[key.front()].item->baseCategory = true;
		rowToCategory.push_back(key.front());
	}

	for (const auto& [id, index] : destructibles_slk.row_headers) {
		BaseTreeItem* item = new BaseTreeItem(getFolderParent(id));
		item->id = id;
		items.emplace(id, item);
	}

	categoryChangeFields = { "category" };
}

BaseTreeItem* DestructibleTreeModel::getFolderParent(const std::string& id) const {
	std::string category = destructibles_slk.data("category", id);

	return categories.at(category.front()).item;
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
				return QAbstractProxyModel::data(index, role).toString() + " " + sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("editorsuffix")), role).toString();
			}
		case Qt::DecorationRole:
			if (item->baseCategory || item->subCategory) {
				return folderIcon;
			}

			return categories.at(rowToCategory[index.parent().row()]).icon->icon;
		default:
			return BaseTreeModel::data(index, role);
	}
}