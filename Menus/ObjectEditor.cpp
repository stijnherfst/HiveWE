#include "ObjectEditor.h"


#include <QTableView>
#include <QLineEdit>
#include <QToolBar>
#include <QDialogButtonBox>
#include <QSortFilterProxyModel>
#include <QPushButton>

#include "SingleModel.h"
#include "UnitSelector.h"

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

	addTypeTreeView(unitTreeModel, unitTreeFilter, units_table, unit_explorer, custom_unit_icon->icon, "Units");
	addTypeTreeView(itemTreeModel, itemTreeFilter, items_table, item_explorer, custom_item_icon->icon, "Items");
	addTypeTreeView(doodadTreeModel, doodadTreeFilter, doodads_table, doodad_explorer, custom_doodad_icon->icon, "Doodads");
	addTypeTreeView(destructibleTreeModel, destructibleTreeFilter, destructibles_table, destructible_explorer, custom_destructible_icon->icon, "Destructibles");
	addTypeTreeView(abilityTreeModel, abilityTreeFilter, abilities_table, ability_explorer, custom_ability_icon->icon, "Abilities");
	addTypeTreeView(upgradeTreeModel, upgradeTreeFilter, upgrade_table, upgrade_explorer, custom_upgrade_icon->icon, "Upgrades");
	addTypeTreeView(buffTreeModel, buffTreeFilter, buff_table, buff_explorer, custom_buff_icon->icon, "Buffs");

	explorer_area->setCurrentIndex(0);

	connect(unit_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(unitTreeFilter, units_table, index, Category::unit); });
	connect(item_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(itemTreeFilter, items_table, index, Category::item); });
	connect(doodad_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(doodadTreeFilter, doodads_table, index, Category::doodad); });
	connect(destructible_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(destructibleTreeFilter, destructibles_table, index, Category::destructible); });
	connect(ability_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(abilityTreeFilter, abilities_table, index, Category::ability); });
	connect(upgrade_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(upgradeTreeFilter, upgrade_table, index, Category::upgrade); });
	connect(buff_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(buffTreeFilter, buff_table, index, Category::buff); });

	show();
}

void ObjectEditor::itemClicked(QSortFilterProxyModel* model, TableModel* table, const QModelIndex& index, Category category) {
	BaseTreeItem* item = static_cast<BaseTreeItem*>(model->mapToSource(index).internalPointer());
	if (item->baseCategory || item->subCategory) {
		return;
	}

	// If there is already one open for this item
	if (auto found = dock_manager->findDockWidget(QString::number(static_cast<int>(category)) + QString::fromStdString(item->id)); found) {
		found->dockAreaWidget()->setCurrentDockWidget(found);
		found->setFocus();
		found->raise();
		return;
	}

	QTableView* view = new QTableView;
	TableDelegate* delegate = new TableDelegate;
	view->setItemDelegate(delegate);
	view->horizontalHeader()->hide();
	view->setAlternatingRowColors(true);
	view->setVerticalHeader(new AlterHeader(Qt::Vertical, view));
	view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed);
	view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
	view->setIconSize({ 24, 24 });

	ads::CDockWidget* dock_tab = new ads::CDockWidget("");
	dock_tab->setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetDeleteOnClose, true);
	dock_tab->setWidget(view);
	dock_tab->setObjectName(QString::number(static_cast<int>(category)) + QString::fromStdString(item->id));
	dock_tab->setWindowTitle(model->data(index, Qt::DisplayRole).toString());
	dock_tab->setIcon(model->data(index, Qt::DecorationRole).value<QIcon>());

	SingleModel* single_model = new SingleModel(table, this);
	single_model->setID(item->id);
	view->setModel(single_model);

	dock_manager->addDockWidget(ads::CenterDockWidgetArea, dock_tab, dock_area);
}

void ObjectEditor::addTypeTreeView(BaseTreeModel* treeModel, BaseFilter*& filter, TableModel* table, QTreeView* view, QIcon icon, QString name) {
	treeModel->setSourceModel(table);
	filter = new BaseFilter;
	filter->slk = table->slk;
	filter->setRecursiveFilteringEnabled(true);
	filter->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
	filter->setSourceModel(treeModel);
	view->setModel(filter);
	view->header()->hide();
	view->setContextMenuPolicy(Qt::CustomContextMenu);
	view->setSelectionBehavior(QAbstractItemView::SelectRows);
	view->setSelectionMode(QAbstractItemView::ExtendedSelection);
	view->setUniformRowHeights(true);
	view->expandAll();

	connect(view, &QTreeView::customContextMenuRequested, [view, name, table, filter, treeModel, this](const QPoint& pos) {
		QMenu menu;
		QAction* addAction = menu.addAction("Add " + name);
		QAction* removeAction = menu.addAction("Remove " + name);

		QModelIndexList selection = view->selectionModel()->selectedIndexes();
		if (selection.empty()) {
			removeAction->setDisabled(true);
		} else {
			BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(filter->mapToSource(selection.front()).internalPointer());
			if (!table->slk->shadow_data.contains(treeItem->id) || !table->slk->shadow_data.at(treeItem->id).contains("oldid")) {
				removeAction->setDisabled(true);	
			}
		}

		connect(addAction, &QAction::triggered, [this, table, treeModel, name, selection]() {
			QDialog* selectdialog = new QDialog(this, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
			selectdialog->resize(300, 560);
			selectdialog->setWindowModality(Qt::WindowModality::WindowModal);

			QLineEdit* nameEdit = new QLineEdit;
			nameEdit->setPlaceholderText("New name");

			QLineEdit* id = new QLineEdit;
			id->setPlaceholderText("Free ID");
			id->setText(QString::fromStdString(table->slk->get_free_row_header(false)));
			id->setFont(QFont("consolas"));

			QHBoxLayout* nameLayout = new QHBoxLayout;
			nameLayout->addWidget(nameEdit, 3);
			nameLayout->addWidget(id, 1);

			BaseFilter* filter = new BaseFilter;
			filter->slk = table->slk;
			filter->setRecursiveFilteringEnabled(true);
			filter->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
			filter->setSourceModel(treeModel);

			QLineEdit* search = new QLineEdit;
			search->setPlaceholderText("Search " + name);
			connect(search, &QLineEdit::textChanged, filter, QOverload<const QString&>::of(&QSortFilterProxyModel::setFilterFixedString));

			QTreeView* view = new QTreeView;
			view->setModel(filter);
			view->setUniformRowHeights(true);
			view->header()->hide();
			view->expandAll();

			QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
			connect(buttonBox, &QDialogButtonBox::accepted, selectdialog, &QDialog::accept);
			connect(buttonBox, &QDialogButtonBox::rejected, selectdialog, &QDialog::reject);

			QVBoxLayout* selectlayout = new QVBoxLayout(selectdialog);
			selectlayout->addLayout(nameLayout);
			selectlayout->addWidget(search);
			selectlayout->addWidget(view);
			selectlayout->addWidget(buttonBox);

			connect(view->selectionModel(), &QItemSelectionModel::currentChanged, [table, filter, id, nameEdit](const QModelIndex& current, const QModelIndex& previous) {
				if (!current.isValid()) {
					return;
				}
				nameEdit->setText(filter->data(current).toString());
				const BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(filter->mapToSource(current).internalPointer());
				id->setText(QString::fromStdString(table->slk->get_free_row_header(!islower(treeItem->id.front()))));
			});

			connect(id, &QLineEdit::textChanged, [buttonBox](const QString& text) {
				buttonBox->button(QDialogButtonBox::Ok)->setEnabled(text.size() == 4);
			});

			auto select = [table, filter, selectdialog, id, nameEdit](const QModelIndex& index) {
				if (id->text().size() != 4) {
					return;
				}

				const BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(filter->mapToSource(index).internalPointer());
				if (treeItem->baseCategory || treeItem->subCategory) {
					return;
				}

				selectdialog->close();
				table->copyRow(treeItem->id, id->text().toStdString());
				table->setData(table->index(table->rowCount() - 1, 0), nameEdit->text());
			};

			connect(view, &QTreeView::activated, [select](const QModelIndex& index) { select(index); });

			connect(selectdialog, &QDialog::accepted, [view, select]() {
				auto indices = view->selectionModel()->selectedIndexes();
				if (indices.empty()) {
					return;
				}

				select(indices.front());
			});

			selectdialog->show();
		});

		connect(removeAction, &QAction::triggered, [table, treeModel, filter, view, selection]() {
			std::vector<std::string> ids_to_delete;
			for (const auto& i : selection) {
				BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(filter->mapToSource(i).internalPointer());
				ids_to_delete.push_back(treeItem->id);
			}
			for (const auto& i : ids_to_delete) {
				table->deleteRow(i);
			}
		});

		menu.exec(view->mapToGlobal(pos));
	});

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
	if (explorer_area == nullptr) {
		explorer_area = dock_manager->addDockWidget(ads::LeftDockWidgetArea, unit_tab);
	} else {
		dock_manager->addDockWidget(ads::CenterDockWidgetArea, unit_tab, explorer_area);
	}
}