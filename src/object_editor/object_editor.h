#pragma once

#include <QMainWindow>

#include "ui_object_editor.h"

#include "DockManager.h"
#include "DockAreaWidget.h"
#include <QTreeView>
#include <QSortFilterProxyModel>

import BaseTreeModel;
import AbilityTreeModel;
import DoodadTreeModel;
import BuffTreeModel;
import DestructibleTreeModel;
import UnitTreeModel;
import UpgradeTreeModel;
import ItemTreeModel;
import TableModel;
import QIconResource;

class ObjectEditor : public QMainWindow {
	Q_OBJECT

public:
	ObjectEditor(QWidget* parent = nullptr);

	enum class Category {
		unit,
		item,
		doodad,
		destructible,
		ability,
		upgrade,
		buff
	};

	void select_id(Category category, std::string id);

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

	void itemClicked(QSortFilterProxyModel* model, TableModel* table, const QModelIndex& index, Category category);
	void addTypeTreeView(BaseTreeModel* treeModel, BaseFilter*& filter, TableModel* table, QTreeView* view, QIcon icon, QString name);
};