#include "ObjectEditor.h"

#include <QSortFilterProxyModel>

#include "SingleModel.h"
#include <QTableView>
#include <QLineEdit>

void ObjectEditor::item_clicked(QSortFilterProxyModel* model, const QModelIndex& index, Category category) {
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

		SingleModel* single_model;
		switch (category) {
			case Category::unit: {
				std::string id = units_slk.index_to_row.at(item->tableRow);

				single_model = new SingleModel(&units_slk, &units_meta_slk, this);
				single_model->setSourceModel(units_table);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(units_slk.data("name", item->tableRow)));
				dock_tab->setIcon(unitTreeModel->data(sourceIndex, Qt::DecorationRole).value<QIcon>());
				break;
			}
			case Category::item: {
				std::string id = items_slk.index_to_row.at(item->tableRow);

				single_model = new SingleModel(&items_slk, &items_meta_slk, this);
				single_model->setSourceModel(items_table);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(items_slk.data("name", item->tableRow)));
				dock_tab->setIcon(itemTreeModel->data(sourceIndex, Qt::DecorationRole).value<QIcon>());
				break;
			}
			case Category::doodad: {
				std::string id = doodads_slk.index_to_row.at(item->tableRow);

				single_model = new SingleModel(&doodads_slk, &doodads_meta_slk, this);
				single_model->setSourceModel(doodads_table);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(doodads_slk.data("name", item->tableRow)));
				dock_tab->setIcon(doodadTreeModel->data(sourceIndex, Qt::DecorationRole).value<QIcon>());
				break;
			}
			case Category::destructible: {
				std::string id = destructibles_slk.index_to_row.at(item->tableRow);

				single_model = new SingleModel(&destructibles_slk, &destructibles_meta_slk, this);
				single_model->setSourceModel(destructibles_table);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(destructibles_slk.data("name", item->tableRow)));
				dock_tab->setIcon(destructibleTreeModel->data(sourceIndex, Qt::DecorationRole).value<QIcon>());
				break;
			}
			case Category::ability: {
				std::string id = abilities_slk.index_to_row.at(item->tableRow);

				single_model = new SingleModel(&abilities_slk, &abilities_meta_slk, this);
				single_model->setSourceModel(abilities_table);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(abilities_slk.data("name", item->tableRow)));
				dock_tab->setIcon(abilityTreeModel->data(sourceIndex, Qt::DecorationRole).value<QIcon>());
				break;
			}
			case Category::upgrade: {
				std::string id = upgrade_slk.index_to_row.at(item->tableRow);

				single_model = new SingleModel(&upgrade_slk, &upgrade_meta_slk, this);
				single_model->setSourceModel(upgrade_table);
				single_model->setID(id);

				dock_tab->setWindowTitle(QString::fromStdString(upgrade_slk.data("name", item->tableRow)));
				dock_tab->setIcon(upgradeTreeModel->data(sourceIndex, Qt::DecorationRole).value<QIcon>());
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
				dock_tab->setIcon(buffTreeModel->data(sourceIndex, Qt::DecorationRole).value<QIcon>());
				break;
			}
			default:
				return;
		}
		view->setModel(single_model);

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

	ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting);
	ads::CDockManager::setConfigFlag(ads::CDockManager::AllTabsHaveCloseButton);
	ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility);
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
	auto addTypeTreeView = [&](BaseTreeModel* treeModel, QSortFilterProxyModel*& filter, TableModel* table, QTreeView* view, QIcon icon, QString name) {
		treeModel->setSourceModel(table);
		filter = new QSortFilterProxyModel;
		filter->setSourceModel(treeModel);
		filter->setRecursiveFilteringEnabled(true);
		filter->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
		view->setModel(filter);
		view->header()->hide();
		view->expandAll();

		ads::CDockWidget* unit_tab = new ads::CDockWidget(name);
		QLineEdit* search = new QLineEdit;
		search->setPlaceholderText("Search " + name);
		connect(search, &QLineEdit::textChanged, filter, QOverload<const QString&>::of(&QSortFilterProxyModel::setFilterFixedString));
		unit_tab->layout()->addWidget(search);
		unit_tab->layout()->addWidget(view);
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

	connect(unit_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(unitTreeFilter, index, Category::unit); });
	connect(item_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(itemTreeFilter, index, Category::item); });
	connect(doodad_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(doodadTreeFilter, index, Category::doodad); });
	connect(destructible_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(destructibleTreeFilter, index, Category::destructible); });
	connect(ability_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(abilityTreeFilter, index, Category::ability); });
	connect(upgrade_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(upgradeTreeFilter, index, Category::upgrade); });
	connect(buff_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { item_clicked(buffTreeFilter, index, Category::buff); });

	show();
}