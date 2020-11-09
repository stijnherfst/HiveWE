#include "ObjectEditor.h"

#include <QSortFilterProxyModel>

#include "SingleModel.h"
#include <QTableView>
#include <QLineEdit>
#include <QToolBar>

void ObjectEditor::item_clicked(QSortFilterProxyModel* model, TableModel* table, const QModelIndex& index, Category category) {
	QModelIndex sourceIndex = model->mapToSource(index);
	BaseTreeItem* item = static_cast<BaseTreeItem*>(sourceIndex.internalPointer());
	if (item->tableRow >= 0) {
		// If there is already one open for this item
		if (auto found = dock_manager->findDockWidget(QString::number(static_cast<int>(category)) + QString::number(item->tableRow)); found) {
			found->dockAreaWidget()->setCurrentDockWidget(found);
			found->setFocus();
			found->raise();
			return;
		}


		ads::CDockWidget* dock_tab = new ads::CDockWidget("");
		dock_tab->setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetDeleteOnClose, true);

		QTableView* view = new QTableView;
		TableDelegate* delegate = new TableDelegate;
		view->setItemDelegate(delegate);
		view->horizontalHeader()->hide();
		view->setAlternatingRowColors(true);
		view->setVerticalHeader(new AlterHeader(Qt::Vertical, view));
		view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed);
		view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
		view->setIconSize({ 24, 24 });
		dock_tab->setWidget(view);
		dock_tab->setObjectName(QString::number(static_cast<int>(category)) + QString::number(item->tableRow));

		SingleModel* single_model = new SingleModel(table, this);
		single_model->setID(table->slk->index_to_row.at(item->tableRow));
		view->setModel(single_model);

		dock_tab->setWindowTitle(model->data(index, Qt::DisplayRole).toString());
		dock_tab->setIcon(model->data(index, Qt::DecorationRole).value<QIcon>());

		dock_manager->addDockWidget(ads::CenterDockWidgetArea, dock_tab, dock_area);
	}
}

ObjectEditor::ObjectEditor(QWidget* parent) : QMainWindow(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	custom_unit_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewUnit"));
	custom_item_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewItem"));
	custom_doodad_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewDood"));
	custom_destructible_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewDest"));
	custom_ability_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewAbil"));
	custom_buff_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewBuff"));
	custom_upgrade_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewUpgr"));

	dock_manager = new ads::CDockManager;
	dock_manager->setStyleSheet("");
	setCentralWidget(dock_manager);

	dock_area = dock_manager->setCentralWidget(new ads::CDockWidget(""));
	dock_area->setAllowedAreas(ads::DockWidgetArea::OuterDockAreas);

	unitTreeModel = new UnitTreeModel(this);
	itemTreeModel = new ItemTreeModel(this);
	doodadTreeModel = new DoodadTreeModel(this);
	destructibleTreeModel = new DestructibleTreeModel(this);
	abilityTreeModel = new AbilityTreeModel(this);
	upgradeTreeModel = new UpgradeTreeModel(this);
	buffTreeModel = new BuffTreeModel(this);
	
	ads::CDockAreaWidget* area = nullptr;
	auto addTypeTreeView = [&](BaseTreeModel* treeModel, BaseFilter*& filter, TableModel* table, QTreeView* view, QIcon icon, QString name) {
		treeModel->setSourceModel(table);
		filter = new BaseFilter;
		filter->slk = table->slk;
		filter->setRecursiveFilteringEnabled(true);
		filter->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
		filter->setSourceModel(treeModel);
		view->setModel(filter);
		view->header()->hide();
		view->expandAll();

		QLineEdit* search = new QLineEdit;
		search->setPlaceholderText("Search " + name);
		connect(search, &QLineEdit::textChanged, filter, QOverload<const QString&>::of(&QSortFilterProxyModel::setFilterFixedString));

		QToolButton* hideDefault = new QToolButton;
		hideDefault->setIcon(icon);
		hideDefault->setToolTip("Hide default " + name);
		hideDefault->setCheckable(true);
		connect(hideDefault, &QToolButton::toggled, filter, &BaseFilter::setFilterCustom);

		QToolBar* bar = new QToolBar;
		bar->addWidget(search);
		bar->addWidget(hideDefault);

		ads::CDockWidget* unit_tab = new ads::CDockWidget(name);
		unit_tab->setToolBar(bar);
		unit_tab->setWidget(view);
		unit_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
		unit_tab->setIcon(icon);
		if (area == nullptr) {
			area = dock_manager->addDockWidget(ads::LeftDockWidgetArea, unit_tab);
		} else {
			dock_manager->addDockWidget(ads::CenterDockWidgetArea, unit_tab, area);
		}
	};

	addTypeTreeView(unitTreeModel, unitTreeFilter, units_table, unit_explorer, custom_unit_icon->icon, "Units");
	addTypeTreeView(itemTreeModel, itemTreeFilter, items_table, item_explorer, custom_item_icon->icon, "Items");
	addTypeTreeView(doodadTreeModel, doodadTreeFilter, doodads_table, doodad_explorer, custom_doodad_icon->icon, "Doodads");
	addTypeTreeView(destructibleTreeModel, destructibleTreeFilter, destructibles_table, destructible_explorer, custom_destructible_icon->icon, "Destructibles");
	addTypeTreeView(abilityTreeModel, abilityTreeFilter, abilities_table, ability_explorer, custom_ability_icon->icon, "Abilities");
	addTypeTreeView(upgradeTreeModel, upgradeTreeFilter, upgrade_table, upgrade_explorer, custom_upgrade_icon->icon, "Upgrades");
	addTypeTreeView(buffTreeModel, buffTreeFilter, buff_table, buff_explorer, custom_buff_icon->icon, "Buffs");

	area->setCurrentIndex(0);

	connect(unit_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(unitTreeFilter, units_table, index, Category::unit); });
	connect(item_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(itemTreeFilter, items_table, index, Category::item); });
	connect(doodad_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(doodadTreeFilter, doodads_table, index, Category::doodad); });
	connect(destructible_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(destructibleTreeFilter, destructibles_table, index, Category::destructible); });
	connect(ability_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(abilityTreeFilter, abilities_table, index, Category::ability); });
	connect(upgrade_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(upgradeTreeFilter, upgrade_table, index, Category::upgrade); });
	connect(buff_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(buffTreeFilter, buff_table, index, Category::buff); });

	show();
}