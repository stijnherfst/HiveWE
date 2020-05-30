#include "UpgradeTreeModel.h"

UpgradeTreeModel::UpgradeTreeModel(QObject* parent) : BaseTreeModel(parent) {
	for (const auto& [key, value] : unit_editor_data.section("unitRace")) {
		if (key == "Sort" || key == "NumValues") {
			continue;
		}

		categories[value[0]].name = value[1];
		categories[value[0]].item = new BaseTreeItem(rootItem);
		categories[value[0]].item->baseCategory = true;
		rowToCategory.push_back(value[0]);
	}

	for (int i = 0; i < upgrade_slk.rows(); i++) {
		std::string race = upgrade_slk.data("race", i);
		if (race.empty()) {
			std::cout << "Empty race for " << i << " in items\n";
			continue;
		}

		BaseTreeItem* item = new BaseTreeItem(categories[race].item);
		item->tableRow = i;
	}
}

QModelIndex UpgradeTreeModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}

	std::string race = upgrade_slk.data("race", sourceIndex.row());

	auto items = categories.at(race).item->children;
	for (int i = 0; i < items.size(); i++) {
		BaseTreeItem* item = items[i];
		if (item->tableRow == sourceIndex.row()) {
			return createIndex(i, 0, item);
		}
	}

	return {};
}

QModelIndex UpgradeTreeModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	BaseTreeItem* item = static_cast<BaseTreeItem*>(proxyIndex.internalPointer());
	if (!item->baseCategory) {
		return createIndex(item->tableRow, upgrade_slk.column_headers.at("name"), item);
	}
	return {};
}

QVariant UpgradeTreeModel::data(const QModelIndex& index, int role) const {
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
				return QAbstractProxyModel::data(index, role).toString() + " " + QString::fromStdString(upgrade_slk.data("editorsuffix", item->tableRow));
			}
		case Qt::DecorationRole:
			if (item->tableRow < 0) {
				return folderIcon;
			}
			return sourceModel()->data(sourceModel()->index(item->tableRow, upgrade_slk.column_headers.at("art")), role);
		default:
			return {};
	}
}