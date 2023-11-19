module;

#include <array>
#include <print>
#include <QMap>
#include <QMargins>
#include <QObject>
#include <QModelIndex>
#include <QSize>
#include <QIcon>
#include <iostream>
#include "ankerl/unordered_dense.h"

#include "globals.h"

export module AbilityTreeModel;

import BaseTreeModel;
import SLK;

export class AbilityTreeModel : public BaseTreeModel {
	struct Category {
		std::string name;
		BaseTreeItem* item;
	};

	std::unordered_map<std::string, Category> categories;
	std::vector<std::string> rowToCategory;

	std::array<std::string, 3> subCategories = {
		"Units",
		"Heroes",
		"Items"
	};

	BaseTreeItem* getFolderParent(const std::string& id) const override {
		std::string race = abilities_slk.data("race", id);
		auto found_race = categories.find(race);
		if (found_race == categories.end()) {
			std::println("Empty or invalid race for ability ID {}, race {}", id, race);
			return nullptr;
		}
		bool isHero = abilities_slk.data<bool>("hero", id);
		bool isItem = abilities_slk.data<bool>("item", id);

		int subIndex = 0;
		if (isHero) {
			subIndex = 1;
		} else if (isItem) {
			subIndex = 2;
		}

		return found_race->second.item->children[subIndex];
	}

  public:
	QVariant data(const QModelIndex& index, int role) const override {
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
					return QAbstractProxyModel::data(index, role).toString() + " " + sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("editorsuffix")), role).toString();
				}
			default:
				return BaseTreeModel::data(index, role);
		}
	}

	explicit AbilityTreeModel(QObject* parent) : BaseTreeModel(parent) {
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
			items.emplace(id, item);
		}

		categoryChangeFields = { "race", "hero", "item" };
	}
};