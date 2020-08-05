#pragma once

#include <array>

#include "HiveWE.h"
#include "BaseTreeModel.h"
#include "QIconResource.h"

class DestructibleTreeModel : public BaseTreeModel {
	struct Category {
		std::string name;
		std::shared_ptr<QIconResource> icon;
		BaseTreeItem* item;
	};

	std::unordered_map<char, Category> categories;
	std::vector<char> rowToCategory;

	QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;

public:
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	explicit DestructibleTreeModel(QObject* parent = nullptr);
};