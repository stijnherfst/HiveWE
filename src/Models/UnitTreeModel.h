#pragma once

#include <array>

#include <QSortFilterProxyModel>

#include "Globals.h"
#include "BaseTreeModel.h"

class UnitTreeModel : public BaseTreeModel {
	Q_OBJECT

	struct Category {
		std::string name;
		BaseTreeItem* item;
	};

	std::unordered_map<std::string, Category> categories;
	std::vector<std::string> rowToCategory;

	std::array<std::string, 4> subCategories = {
		"Units",
		"Buildings",
		"Heroes",
		"Special",
	};

	BaseTreeItem* getFolderParent(const std::string& id) const override;
	
  public:
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	explicit UnitTreeModel(QObject* parent = nullptr);
};