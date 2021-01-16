#pragma once

#include <array>

#include "HiveWE.h"
#include "BaseTreeModel.h"

class ItemTreeModel : public BaseTreeModel {
	struct Category {
		std::string name;
		BaseTreeItem* item;
	};

	std::unordered_map<std::string, Category> categories;
	std::vector<std::string> rowToCategory;

	BaseTreeItem* getFolderParent(const std::string& id) const override;

public:
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	explicit ItemTreeModel(QObject* parent = nullptr);
};