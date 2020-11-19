#include "UnitTreeModel.h"

UnitTreeModel::UnitTreeModel(QObject* parent) : BaseTreeModel(parent) {
	slk = &units_slk;

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
		const std::string id = units_slk.index_to_row.at(i); 
		BaseTreeItem* item = new BaseTreeItem(getFolderParent(id));
		item->id = id;
	}
}

BaseTreeItem* UnitTreeModel::getFolderParent(const std::string& id) const {
	std::string race = units_slk.data("race", id);
	bool isBuilding = units_slk.data("isbldg", id) == "1";
	bool isHero = isupper(id.front());
	bool isSpecial = units_slk.data("special", id) == "1";

	int subIndex = 0;
	if (isSpecial) {
		subIndex = 3;
	} else if (isBuilding) {
		subIndex = 1;
	} else if (isHero) {
		subIndex = 2;
	}

	return categories.at(race).item->children[subIndex];
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
				return QString::fromStdString(subCategories[index.row()] + " (" + std::to_string(item->children.size()) + ")");
			} else {
				if (units_slk.data("campaign", item->id) == "1") {
					const std::string properNames = units_slk.data("propernames", item->id);

					if (!properNames.empty()) {
						return QString::fromStdString(properNames).split(',').first();
					}
				}

				return QAbstractProxyModel::data(index, role).toString() + " " + QString::fromStdString(units_slk.data("editorsuffix", item->id));
			}
		default:
			return BaseTreeModel::data(index, role);
	}
}