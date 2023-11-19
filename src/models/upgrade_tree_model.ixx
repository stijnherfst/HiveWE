module;

#include <array>
#include <QMap>
#include <QMargins>
#include <QObject>
#include <QModelIndex>
#include <QSize>
#include <QIcon>
#include <iostream>
#include "ankerl/unordered_dense.h"

#include "globals.h"

export module UpgradeTreeModel;

import BaseTreeModel;
import SLK;

export class UpgradeTreeModel : public BaseTreeModel {
	struct Category {
		std::string name;
		BaseTreeItem* item;
	};

	std::unordered_map<std::string, Category> categories;
	std::vector<std::string> rowToCategory;

	BaseTreeItem* getFolderParent(const std::string& id) const override {
		std::string race = upgrade_slk.data("race", id);
		if (race.empty()) {
			std::cout << "Empty race for " << id << " in items\n";
			return nullptr;
		}

		return categories.at(race).item;
	}

	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override {
		if (!proxyIndex.isValid()) {
			return {};
		}

		BaseTreeItem* item = static_cast<BaseTreeItem*>(proxyIndex.internalPointer());

		if (item->baseCategory || item->subCategory) {
			return {};
		}

		return createIndex(slk->row_headers.at(item->id), slk->column_headers.at("name1"), item);
	}

  public:
	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid()) {
			return {};
		}

		BaseTreeItem* item = static_cast<BaseTreeItem*>(index.internalPointer());

		switch (role) {
			case Qt::DecorationRole:
				if (item->baseCategory || item->subCategory) {
					return folderIcon;
				}
				return sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("art1")), role);
			case Qt::EditRole:
			case Qt::DisplayRole:
				if (item->baseCategory) {
					return QString::fromStdString(categories.at(rowToCategory[index.row()]).name);
				} else {
					return QAbstractProxyModel::data(index, role).toString() + " " + QString::fromStdString(upgrade_slk.data("editorsuffix", item->id));
				}
			default:
				return BaseTreeModel::data(index, role);
		}
	}

	explicit UpgradeTreeModel(QObject* parent)
		: BaseTreeModel(parent) {
		slk = &upgrade_slk;

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
			const std::string& id = upgrade_slk.index_to_row.at(i);

			std::string race = upgrade_slk.data("race", id);
			if (race.empty()) {
				std::cout << "Empty race for " << i << " in items\n";
				continue;
			}

			BaseTreeItem* parent_item = getFolderParent(id);
			if (!parent_item) {
				continue;
			}

			BaseTreeItem* item = new BaseTreeItem(parent_item);
			item->id = id;
			items.emplace(id, item);
		}

		categoryChangeFields = { "race" };
	}
};