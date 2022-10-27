#pragma once

#include <array>

#include "BaseTreeModel.h"

class UpgradeTreeModel : public BaseTreeModel {
	struct Category {
		std::string name;
		BaseTreeItem* item;
	};

	std::unordered_map<std::string, Category> categories;
	std::vector<std::string> rowToCategory;

	BaseTreeItem* getFolderParent(const std::string& id) const override;

	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;

public:
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	explicit UpgradeTreeModel(QObject* parent = nullptr);
};