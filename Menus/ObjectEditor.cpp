#include "ObjectEditor.h"

#include <QSortFilterProxyModel>

#include "SingleModel.h"
#include <QTableView>

void ObjectEditor::item_clicked(const QModelIndex& index, Category category) {
	BaseTreeItem* item = static_cast<BaseTreeItem*>(index.internalPointer());
	if (item->tableRow > 0) {
		ads::CDockWidget* dock_tab = new ads::CDockWidget("");
		dock_tab->setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetDeleteOnClose, true);
		
		connect(dock_tab, &ads::CDockWidget::closeRequested, [&, dock_tab]() {
			if (dock_area->dockWidgets().contains(dock_tab) && dock_area->dockWidgetsCount() == 1) {
				dock_area = nullptr;
			}
		});

		

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

		SingleModel* single_model;
		switch (category) {
			case Category::unit: {
				std::string id = units_slk.index_to_row.at(item->tableRow);

				single_model = new SingleModel(&units_slk, &units_meta_slk, this);
				single_model->setSourceModel(units_table);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(units_slk.data("name", item->tableRow)));
				dock_tab->setIcon(unitTreeModel->data(index, Qt::DecorationRole).value<QIcon>());
				break;
			}
			case Category::item: {
				std::string id = items_slk.index_to_row.at(item->tableRow);

				single_model = new SingleModel(&items_slk, &items_meta_slk, this);
				single_model->setSourceModel(items_table);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(items_slk.data("name", item->tableRow)));
				dock_tab->setIcon(itemTreeModel->data(index, Qt::DecorationRole).value<QIcon>());
				break;
			}
			case Category::doodad: {
				std::string id = doodads_slk.index_to_row.at(item->tableRow);

				single_model = new SingleModel(&doodads_slk, &doodads_meta_slk, this);
				single_model->setSourceModel(doodads_table);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(doodads_slk.data("name", item->tableRow)));
				dock_tab->setIcon(doodadTreeModel->data(index, Qt::DecorationRole).value<QIcon>());
				break;
			}
			case Category::ability: {
				std::string id = abilities_slk.index_to_row.at(item->tableRow);


				single_model = new SingleModel(&abilities_slk, &abilities_meta_slk, this);
				single_model->setSourceModel(abilities_table);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(abilities_slk.data("name", item->tableRow)));
				dock_tab->setIcon(abilityTreeModel->data(index, Qt::DecorationRole).value<QIcon>());
				break;
			}
			case Category::upgrade: {
				std::string id = upgrade_slk.index_to_row.at(item->tableRow);

				single_model = new SingleModel(&upgrade_slk, &upgrade_meta_slk, this);
				single_model->setSourceModel(upgrade_table);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(upgrade_slk.data("name", item->tableRow)));
				dock_tab->setIcon(upgradeTreeModel->data(index, Qt::DecorationRole).value<QIcon>());
				break;
			}
			case Category::buff: {
				std::string id = buff_slk.index_to_row.at(item->tableRow);

				single_model = new SingleModel(&buff_slk, &buff_meta_slk, this);
				single_model->setSourceModel(buff_table);
				single_model->setID(id);

				std::string name = buff_slk.data("bufftip", item->tableRow);
				if (name.empty()) {
					name = buff_slk.data("editorname", item->tableRow);
				}

				dock_tab->setWindowTitle(QString::fromStdString(name));
				dock_tab->setIcon(buffTreeModel->data(index, Qt::DecorationRole).value<QIcon>());
				break;
			}
			default:
				return;
		}
		view->setModel(single_model);

		if (dock_area == nullptr) {
			dock_area = dock_manager->addDockWidget(ads::RightDockWidgetArea, dock_tab, dock_area);
		} else {
			dock_manager->addDockWidget(ads::CenterDockWidgetArea, dock_tab, dock_area);
		}
	}
}

ObjectEditor::ObjectEditor(QWidget* parent) : QMainWindow(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	custom_unit_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewUnit"));
	custom_item_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewItem"));
	custom_doodad_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewDood"));
	custom_destructable_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewDest"));
	custom_ability_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewAbil"));
	custom_buff_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewBuff"));
	custom_upgrade_icon = resource_manager.load<QIconResource>(world_edit_data.data("WorldEditArt", "ToolBarIcon_OE_NewUpgr"));

	dock_manager->setConfigFlag(ads::CDockManager::eConfigFlag::AllTabsHaveCloseButton);
	dock_manager->setConfigFlag(ads::CDockManager::eConfigFlag::DockAreaDynamicTabsMenuButtonVisibility);
	dock_manager->setStyleSheet("");
	setCentralWidget(dock_manager);

	// Units
	unitTreeModel = new UnitTreeModel(this);
	unitTreeModel->setSourceModel(units_table);
	unit_explorer->setModel(unitTreeModel);
	unit_explorer->header()->hide();
	unit_explorer->expand(unit_explorer->currentIndex());

	ads::CDockWidget* unit_tab = new ads::CDockWidget("Units");
	unit_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	unit_tab->setWidget(unit_explorer);
	unit_tab->setIcon(custom_unit_icon->icon);
	auto t = dock_manager->addDockWidget(ads::LeftDockWidgetArea, unit_tab);
	
	// Items
	itemTreeModel = new ItemTreeModel(this);
	itemTreeModel->setSourceModel(items_table);
	item_explorer->setModel(itemTreeModel);
	item_explorer->header()->hide();

	ads::CDockWidget* item_tab = new ads::CDockWidget("Items");
	item_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	item_tab->setWidget(item_explorer);
	item_tab->setIcon(custom_item_icon->icon);
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, item_tab, t);

	// Doodads
	doodadTreeModel = new DoodadTreeModel(this);
	doodadTreeModel->setSourceModel(doodads_table);
	doodad_explorer->setModel(doodadTreeModel);
	doodad_explorer->header()->hide();

	ads::CDockWidget* doodad_tab = new ads::CDockWidget("Doodads");
	doodad_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	doodad_tab->setWidget(doodad_explorer);
	doodad_tab->setIcon(custom_doodad_icon->icon);
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, doodad_tab, t);

	// Destructables
	ads::CDockWidget* destructable_tab = new ads::CDockWidget("Destructables");
	destructable_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	destructable_tab->setIcon(custom_destructable_icon->icon);
	//destructible_tab->setWidget(explorer);
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, destructable_tab, t);

	// Abilities
	abilityTreeModel = new AbilityTreeModel(this);
	abilityTreeModel->setSourceModel(abilities_table);
	ability_explorer->setModel(abilityTreeModel);
	ability_explorer->header()->hide();

	ads::CDockWidget* ability_tab = new ads::CDockWidget("Abilities");
	ability_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	ability_tab->setWidget(ability_explorer);
	ability_tab->setIcon(custom_ability_icon->icon);
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, ability_tab, t);

	// Upgrades
	upgradeTreeModel = new UpgradeTreeModel(this);
	upgradeTreeModel->setSourceModel(upgrade_table);
	upgrade_explorer->setModel(upgradeTreeModel);
	upgrade_explorer->header()->hide();

	ads::CDockWidget* upgrade_tab = new ads::CDockWidget("Upgrades");
	upgrade_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	upgrade_tab->setWidget(upgrade_explorer);
	upgrade_tab->setIcon(custom_upgrade_icon->icon);
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, upgrade_tab, t);

	buffTreeModel = new BuffTreeModel(this);
	buffTreeModel->setSourceModel(buff_table);
	buff_explorer->setModel(buffTreeModel);
	buff_explorer->header()->hide();

	// Buffs
	ads::CDockWidget* buff_tab = new ads::CDockWidget("Buffs");
	buff_tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	buff_tab->setWidget(buff_explorer);
	buff_tab->setIcon(custom_buff_icon->icon);
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, buff_tab, t);

	t->setCurrentIndex(0);

	connect(unit_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(index, Category::unit); });
	connect(item_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(index, Category::item); });
	connect(doodad_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(index, Category::doodad); });
	connect(ability_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(index, Category::ability); });
	connect(upgrade_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(index, Category::upgrade); });
	connect(buff_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(index, Category::buff); });

	show();
}