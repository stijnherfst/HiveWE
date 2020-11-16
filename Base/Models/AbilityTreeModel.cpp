#include "AbilityTreeModel.h"

AbilityTreeModel::AbilityTreeModel(QObject* parent) : BaseTreeModel(parent) {
	slk = &abilities_slk;

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

	for (int i = 0; i < abilities_slk.rows(); i++) {
		const std::string& id = abilities_slk.index_to_row.at(i);

		BaseTreeItem* parent_item = getFolderParent(id);
		if (!parent_item) {
			continue;
		}

		BaseTreeItem* item = new BaseTreeItem(parent_item);
		item->id = id;
	}
}

BaseTreeItem* AbilityTreeModel::getFolderParent(const std::string& id) const {
	std::string race = abilities_slk.data("race", id);
	if (race.empty()) {
		std::cout << "Empty race for " << id << " in abilities\n";
		return nullptr;
	}
	bool isHero = abilities_slk.data("hero", id) == "1";
	bool isItem = abilities_slk.data("item", id) == "1";

	int subIndex = 0;
	if (isHero) {
		subIndex = 1;
	} else if (isItem) {
		subIndex = 2;
	}

	return categories.at(race).item->children[subIndex];
}

QVariant AbilityTreeModel::data(const QModelIndex& index, int role) const {
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
				return QAbstractProxyModel::data(index, role).toString() + " " + QString::fromStdString(abilities_slk.data("editorsuffix", item->id));
			}
		case Qt::DecorationRole:
			if (item->baseCategory || item->subCategory) {
				return folderIcon;
			}
			return sourceModel()->data(sourceModel()->index(abilities_slk.row_headers.at(item->id), abilities_slk.column_headers.at("art")), role);
		case Qt::TextColorRole:
			if (item->baseCategory || item->subCategory) {
				return {};
			}

			if (abilities_slk.shadow_data.contains(item->id)) {
				return QColor("violet");
			} else {
				return {};
			}
		default:
			return {};
	}
}