#include "BuffTreeModel.h"

BuffTreeModel::BuffTreeModel(QObject* parent) : BaseTreeModel(parent) {
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

	for (int i = 0; i < buff_slk.rows(); i++) {
		std::string race = buff_slk.data("race", i);
		if (race.empty()) {
			std::cout << "Empty race for " << i << " in buffs\n";
			continue;
		}
		bool isEffect = buff_slk.data("iseffect", i) == "1";

		int subIndex = isEffect ? 1 : 0;

		BaseTreeItem* item = new BaseTreeItem(categories[race].item->children[subIndex]);
		item->tableRow = i;
	}
}

QModelIndex BuffTreeModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}

	std::string race = buff_slk.data("race", sourceIndex.row());
	bool isEffect = buff_slk.data("iseffect", sourceIndex.row()) == "1";

	int subIndex = isEffect ? 1 : 0;

	auto& items = categories.at(race).item->children[subIndex]->children;
	for (int i = 0; i < items.size(); i++) {
		BaseTreeItem* item = items[i];
		if (item->tableRow == sourceIndex.row()) {
			return createIndex(i, 0, item);
		}
	}

	return {};
}

QModelIndex BuffTreeModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	BaseTreeItem* item = static_cast<BaseTreeItem*>(proxyIndex.internalPointer());
	if (!item->baseCategory && !item->subCategory) {
		return createIndex(item->tableRow, buff_slk.column_headers.at("bufftip"), item);
	}
	return {};
}

QVariant BuffTreeModel::data(const QModelIndex& index, int role) const {
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
				QString name = QAbstractProxyModel::data(index, role).toString();
				if (name.isEmpty()) {
					return QString::fromStdString(buff_slk.data("editorname", item->tableRow) + " " + buff_slk.data("editorsuffix", item->tableRow));
				} else {
					return name + " " + QString::fromStdString(buff_slk.data("editorsuffix", item->tableRow));
				}
			}
		case Qt::DecorationRole:
			if (item->baseCategory || item->subCategory) {
				return folderIcon;
			}
			return sourceModel()->data(sourceModel()->index(item->tableRow, buff_slk.column_headers.at("buffart")), role);
		case Qt::TextColorRole:
			if (item->baseCategory || item->subCategory) {
				return {};
			}

			if (buff_slk.shadow_data.contains(buff_slk.index_to_row.at(item->tableRow))) {
				return QColor("violet");
			} else {
				return {};
			}
		default:
			return {};
	}
}