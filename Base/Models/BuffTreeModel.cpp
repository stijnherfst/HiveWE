#include "BuffTreeModel.h"

BuffTreeModel::BuffTreeModel(QObject* parent) : BaseTreeModel(parent) {
	slk = &buff_slk;

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
		const std::string& id = buff_slk.index_to_row.at(i);

		BaseTreeItem* parent_item = getFolderParent(id);
		if (!parent_item) {
			continue;
		}

		BaseTreeItem* item = new BaseTreeItem(parent_item);
		item->id = id;
	}
}

BaseTreeItem* BuffTreeModel::getFolderParent(const std::string& id) const {
	std::string race = buff_slk.data("race", id);
	if (race.empty()) {
		std::cout << "Empty race for " << id << " in buffs\n";
		return nullptr;
	}
	bool isEffect = buff_slk.data("iseffect", id) == "1";

	int subIndex = isEffect ? 1 : 0;

	return categories.at(race).item->children[subIndex];
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
					return QString::fromStdString(buff_slk.data("editorname", item->id) + " " + buff_slk.data("editorsuffix", item->id));
				} else {
					return name + " " + QString::fromStdString(buff_slk.data("editorsuffix", item->id));
				}
			}
		case Qt::DecorationRole:
			if (item->baseCategory || item->subCategory) {
				return folderIcon;
			}
			return sourceModel()->data(sourceModel()->index(buff_slk.row_headers.at(item->id), buff_slk.column_headers.at("buffart")), role);
		case Qt::TextColorRole:
			if (item->baseCategory || item->subCategory) {
				return {};
			}

			if (buff_slk.shadow_data.contains(item->id)) {
				return QColor("violet");
			} else {
				return {};
			}
		default:
			return {};
	}
}