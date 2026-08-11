module;

#include <QMap>
#include <QObject>
#include <QModelIndex>
#include <QIcon>

export module DoodadTreeModel;

import std;
import BaseTreeModel;
import QIconResource;
import SLK;
import Globals;
import ResourceManager;

export class DoodadTreeModel : public BaseTreeModel {
	struct Category {
		std::string name;
		std::shared_ptr<QIconResource> icon;
		BaseTreeItem* item;
	};

	std::unordered_map<char, Category> categories;
	std::vector<char> rowToCategory;

	BaseTreeItem* getFolderParent(const std::string& id) const override {
		const std::string_view category = doodads_slk.data<std::string_view>("category", id);

		if (category.empty()) {
			std::println("Doodad with id: {} has no category set. Set a category!", id);
			return categories.begin()->second.item;
		}

		if (const auto found = categories.find(category.front()); found != categories.end()) {
			return found->second.item;
		}

		return categories.begin()->second.item;
	}

	static constexpr char cliffCategory = 'C';

	DropChange prepareDrop(const std::string& id, const BaseTreeItem* target) const override {
		if (!target->baseCategory) {
			return {};
		}

		const char category = rowToCategory[target->row()];
		const std::string_view current = doodads_slk.data<std::string_view>("category", id);

		const bool wasCliff = !current.empty() && current.front() == cliffCategory;
		const bool isCliff = category == cliffCategory;

		std::vector<std::pair<std::string, std::string>> fields = { { "category", std::string(1, category) } };

		if (wasCliff != isCliff) {
			return { DropChange::Verdict::confirm,
					 "Moving doodads into or out of the Cliff/Terrain category is a functional change.\n\nAre you sure?",
					 fields };
		}

		return { DropChange::Verdict::accept, {}, fields };
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
					return QAbstractProxyModel::data(index, role).toString();
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

	explicit DoodadTreeModel(QObject* parent)
		: BaseTreeModel(parent) {
		slk = &doodads_slk;

		for (const auto& [key, value] : world_edit_data.section("DoodadCategories")) {
			categories[key.front()].name = value[0];
			categories[key.front()].icon = resource_manager.load<QIconResource>(value[1]).value();
			categories[key.front()].item = new BaseTreeItem(rootItem);
			categories[key.front()].item->baseCategory = true;
			rowToCategory.push_back(key.front());
		}

		for (size_t i = 0; i < doodads_slk.rows(); i++) {
			const std::string& id = doodads_slk.index_to_row.at(i);
			BaseTreeItem* item = new BaseTreeItem(getFolderParent(id));
			item->id = id;
			items.emplace(id, item);
		}

		categoryChangeFields = { "category" };
		mimeType = "application/x-hivewe-doodads";
	}
};