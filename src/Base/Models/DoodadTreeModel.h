#pragma once

#include <array>

#include "HiveWE.h"
#include "BaseTreeModel.h"
#include "QIconResource.h"

class DoodadTreeModel : public BaseTreeModel {
	struct Category {
		std::string name;
		std::shared_ptr<QIconResource> icon;
		BaseTreeItem* item;
	};


	std::unordered_map<char, Category> categories;
	std::vector<char> rowToCategory;

	BaseTreeItem* getFolderParent(const std::string& id) const override;

public:
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	explicit DoodadTreeModel(QObject* parent = nullptr);
};