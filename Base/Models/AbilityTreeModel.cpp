#include "AbilityTreeModel.h"

AbilityTreeModel::AbilityTreeModel(QObject* parent) : BaseTreeModel(parent) {
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
		std::string race = abilities_slk.data("race", i);
		if (race.empty()) {
			std::cout << "Empty race for " << i << " in abilities\n";
			continue;
		}
		bool isHero = abilities_slk.data("hero", i) == "1";
		bool isItem = abilities_slk.data("item", i) == "1";

		int subIndex = 0;
		if (isHero) {
			subIndex = 1;
		} else if (isItem) {
			subIndex = 2;
		}

		BaseTreeItem* item = new BaseTreeItem(categories[race].item->children[subIndex]);
		item->tableRow = i;
	}
}

QModelIndex AbilityTreeModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}

	std::string race = abilities_slk.data("race", sourceIndex.row());
	bool isHero = abilities_slk.data("hero", sourceIndex.row()) == "1";
	bool isItem = abilities_slk.data("item", sourceIndex.row()) == "1";

	int subIndex = 0;
	if (isHero) {
		subIndex = 1;
	} else if (isItem) {
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

QModelIndex AbilityTreeModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	BaseTreeItem* item = static_cast<BaseTreeItem*>(proxyIndex.internalPointer());
	if (!item->baseCategory && !item->subCategory) {
		return createIndex(item->tableRow, abilities_slk.column_headers.at("name"), item);
	}
	return {};
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
				return QAbstractProxyModel::data(index, role).toString() + " " + QString::fromStdString(abilities_slk.data("editorsuffix", item->tableRow));
			}
		case Qt::DecorationRole:
			if (item->tableRow < 0) {
				return folderIcon;
			}
			return sourceModel()->data(sourceModel()->index(item->tableRow, abilities_slk.column_headers.at("art")), role);
		case Qt::TextColorRole:
			if (item->tableRow < 0) {
				return {};
			}

			if (abilities_slk.shadow_data.contains(abilities_slk.index_to_row.at(item->tableRow))) {
				return QColor("violet");
			} else {
				return {};
			}
		default:
			return {};
	}
}