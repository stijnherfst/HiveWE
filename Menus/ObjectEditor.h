#pragma once

#include <QMainWindow>

#include "ui_ObjectEditor.h"

#include "TableModel.h"
#include "UnitTreeModel.h"
#include "DoodadTreeModel.h"
#include "DestructibleTreeModel.h"
#include "AbilityTreeModel.h"
#include "ItemTreeModel.h"
#include "BuffTreeModel.h"
#include "UpgradeTreeModel.h"
#include "DockManager.h"
#include "DockAreaWidget.h"
#include <QTreeView>

class ObjectEditor : public QMainWindow {
	Q_OBJECT

public:
	ObjectEditor(QWidget* parent = nullptr);

private:
	Ui::ObjectEditor ui;

	ads::CDockManager* dock_manager;
	ads::CDockAreaWidget* dock_area = nullptr;
	ads::CDockAreaWidget* explorer_area = nullptr;

	QTreeView* unit_explorer = new QTreeView;
	QTreeView* doodad_explorer = new QTreeView;
	QTreeView* item_explorer = new QTreeView;
	QTreeView* destructible_explorer = new QTreeView;
	QTreeView* ability_explorer = new QTreeView;
	QTreeView* upgrade_explorer = new QTreeView;
	QTreeView* buff_explorer = new QTreeView;
	
	UnitTreeModel* unitTreeModel;
	DoodadTreeModel* doodadTreeModel;
	DestructibleTreeModel* destructibleTreeModel;
	AbilityTreeModel* abilityTreeModel;
	ItemTreeModel* itemTreeModel;
	BuffTreeModel* buffTreeModel;
	UpgradeTreeModel* upgradeTreeModel;

	BaseFilter* unitTreeFilter;
	BaseFilter* doodadTreeFilter;
	BaseFilter* destructibleTreeFilter;
	BaseFilter* abilityTreeFilter;
	BaseFilter* itemTreeFilter;
	BaseFilter* buffTreeFilter;
	BaseFilter* upgradeTreeFilter;

	std::shared_ptr<QIconResource> custom_unit_icon;
	std::shared_ptr<QIconResource> custom_item_icon;
	std::shared_ptr<QIconResource> custom_doodad_icon;
	std::shared_ptr<QIconResource> custom_destructible_icon;
	std::shared_ptr<QIconResource> custom_ability_icon;
	std::shared_ptr<QIconResource> custom_buff_icon;
	std::shared_ptr<QIconResource> custom_upgrade_icon;

	enum class Category {
		unit,
		doodad,
		item,
		destructible,
		ability,
		upgrade,
		buff
	};

	void item_clicked(QSortFilterProxyModel* model, TableModel* table, const QModelIndex& index, Category category);

	void add_item();

	void addTypeTreeView(BaseTreeModel* treeModel, BaseFilter*& filter, TableModel* table, QTreeView* view, QIcon icon, QString name);
};