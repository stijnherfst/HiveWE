#include "UnitTreeModel.h"

UnitTreeModel::UnitTreeModel(QObject* parent) : BaseTreeModel(parent) {
	for (const auto& [key, value] : unit_editor_data.section("unitRace")) {
		if (key == "Sort" || key == "NumValues") {
			continue;
		}

		categories[value[0]].name = value[1];
		categories[value[0]].item = new BaseTreeItem(rootItem);
		categories[value[0]].item->baseCategory = true;
		rowToCategory.push_back(value[0]);
	}

	for (const auto& i : rootItem->children) {
		for (const auto& subCategory : subCategories) {
			BaseTreeItem* item = new BaseTreeItem(i);
			item->subCategory = true;
		}
	}


	for (int i = 0; i < units_slk.rows(); i++) {
		std::string race = units_slk.data("race", i);
		bool isBuilding = units_slk.data("isbldg", i) == "1";
		bool isHero = isupper(units_slk.index_to_row.at(i).front());
		bool isSpecial = units_slk.data("special", i) == "1";

		int subIndex = 0;
		if (isSpecial) {
			subIndex = 3;
		} else if (isBuilding) {
			subIndex = 1;
		} else if (isHero) {
			subIndex = 2;
		}

		BaseTreeItem* item = new BaseTreeItem(categories[race].item->children[subIndex]);
		item->tableRow = i;
	}
}

QModelIndex UnitTreeModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}

	std::string race = units_slk.data("race", sourceIndex.row());
	bool isBuilding = units_slk.data("isbldg", sourceIndex.row()) == "1";
	bool isHero = isupper(units_slk.index_to_row.at(sourceIndex.row()).front());
	bool isSpecial = units_slk.data("special", sourceIndex.row()) == "1";

	int subIndex = 0;
	if (isSpecial) {
		subIndex = 3;
	} else if (isBuilding) {
		subIndex = 1;
	} else if (isHero) {
		subIndex = 2;
	}

	auto items = categories.at(race).item->children[subIndex]->children;
	for (int i = 0; i < items.size(); i++) {
		BaseTreeItem* item = items[i];
		if (item->tableRow == sourceIndex.row()) {
			return createIndex(i, 0, item);
		}
	}

	return {};
}

QModelIndex UnitTreeModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	BaseTreeItem* item = static_cast<BaseTreeItem*>(proxyIndex.internalPointer());
	if (!item->baseCategory && !item->subCategory) {
		return createIndex(item->tableRow, units_slk.column_headers.at("name"), item);
	}
	return {};
}

QVariant UnitTreeModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return {};
	}

	BaseTreeItem* item = static_cast<BaseTreeItem*>(index.internalPointer());

	switch (role) {
		case Qt::EditRole:
		case Qt::DisplayRole:
			if (item->baseCategory) {
				return QString::fromStdString(categories.at(rowToCategory[index.row()]).name);
			} else if (item->subCategory) {
				return QString::fromStdString(subCategories[index.row()]);
			} else {
				if (units_slk.data("campaign", item->tableRow) == "1") {
					const std::string properNames = units_slk.data("propernames", item->tableRow);

					if (!properNames.empty()) {
						return QString::fromStdString(properNames).split(',').first();
					}
				}

				return QAbstractProxyModel::data(index, role).toString() + " " + QString::fromStdString(units_slk.data("editorsuffix", item->tableRow));
			}
		case Qt::DecorationRole:
			if (item->tableRow < 0) {
				return folderIcon;
			}
			return sourceModel()->data(sourceModel()->index(item->tableRow, units_slk.column_headers.at("art")), role);
		default:
			return {};
	}
}