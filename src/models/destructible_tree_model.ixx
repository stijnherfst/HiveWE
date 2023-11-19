module;

#include <array>
#include <QMap>
#include <QMargins>
#include <QObject>
#include <QModelIndex>
#include <QSize>
#include <QIcon>
#include "ankerl/unordered_dense.h"

#include "globals.h"

export module DestructibleTreeModel;

import BaseTreeModel;
import QIconResource;
import SLK;

export class DestructibleTreeModel : public BaseTreeModel {
	struct Category {
		std::string name;
		std::shared_ptr<QIconResource> icon;
		BaseTreeItem* item;
	};

	std::unordered_map<char, Category> categories;
	std::vector<char> rowToCategory;

	BaseTreeItem* getFolderParent(const std::string& id) const override {
		std::string category = destructibles_slk.data("category", id);

		return categories.at(category.front()).item;
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

	explicit DestructibleTreeModel(QObject* parent)
		: BaseTreeModel(parent) {
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
};