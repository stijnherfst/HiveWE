#include "object_editor.h"

#include <QTableView>
#include <QLineEdit>
#include <QToolBar>
#include <QDialogButtonBox>
#include <QSortFilterProxyModel>
#include <QAbstractProxyModel>
#include <QPushButton>
#include <QTimer>
#include <QLabel>
#include <QShortcut>
#include <QDialog>
#include <QToolButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QMenu>

import std;
import UnitSelector;
import MapGlobal;
import Globals;
import ResourceManager;
import SlkConversions;
import "single_model.h";

ObjectEditor::ObjectEditor(QWidget* parent) : QMainWindow(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	std::ifstream f("data/warcraft/ability_insights.json");
	ability_insights = nlohmann::json::parse(f);

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

	QLabel* image = new QLabel();
	image->setPixmap(QPixmap("data/icons/object_editor/background.png"));
	image->setAlignment(Qt::AlignCenter);

	auto centraldock_widget = new ads::CDockWidget(dock_manager, "CentralWidget");
	centraldock_widget->setWidget(image);
	centraldock_widget->setFeature(ads::CDockWidget::NoTab, true);
	dock_area = dock_manager->setCentralWidget(centraldock_widget);

	unitTreeModel = new UnitTreeModel(this);
	itemTreeModel = new ItemTreeModel(this);
	doodadTreeModel = new DoodadTreeModel(this);
	destructibleTreeModel = new DestructibleTreeModel(this);
	abilityTreeModel = new AbilityTreeModel(this);
	upgradeTreeModel = new UpgradeTreeModel(this);
	buffTreeModel = new BuffTreeModel(this);

	addTypeTreeView(unitTreeModel, unitTreeFilter, units_table, unit_explorer, custom_unit_icon->icon, "Units", Category::unit);
	addTypeTreeView(itemTreeModel, itemTreeFilter, items_table, item_explorer, custom_item_icon->icon, "Items", Category::item);
	addTypeTreeView(doodadTreeModel, doodadTreeFilter, doodads_table, doodad_explorer, custom_doodad_icon->icon, "Doodads", Category::doodad);
	addTypeTreeView(destructibleTreeModel, destructibleTreeFilter, destructibles_table, destructible_explorer, custom_destructible_icon->icon, "Destructibles", Category::destructible);
	addTypeTreeView(abilityTreeModel, abilityTreeFilter, abilities_table, ability_explorer, custom_ability_icon->icon, "Abilities", Category::ability);
	addTypeTreeView(upgradeTreeModel, upgradeTreeFilter, upgrade_table, upgrade_explorer, custom_upgrade_icon->icon, "Upgrades", Category::upgrade);
	addTypeTreeView(buffTreeModel, buffTreeFilter, buff_table, buff_explorer, custom_buff_icon->icon, "Buffs", Category::buff);

	explorer_area->setCurrentIndex(0);
	// Set initial sizes, the second size doesn't really matter with only 2 dock areas
	dock_manager->setSplitterSizes(explorer_area, { 645, 9999 });

	connect(unit_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(unitTreeFilter, units_table, index); });
	connect(item_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(itemTreeFilter, items_table, index); });
	connect(doodad_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(doodadTreeFilter, doodads_table, index); });
	connect(destructible_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(destructibleTreeFilter, destructibles_table, index); });
	connect(ability_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(abilityTreeFilter, abilities_table, index); });
	connect(upgrade_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(upgradeTreeFilter, upgrade_table, index); });
	connect(buff_explorer, &QTreeView::doubleClicked, [&](const QModelIndex& index) { itemClicked(buffTreeFilter, buff_table, index); });

	connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this), &QShortcut::activated, [&]() {
		auto edit = explorer_area->currentDockWidget()->findChild<QLineEdit*>("search");
		edit->setFocus();
		edit->selectAll();
	});

	show();
}

void ObjectEditor::itemClicked(const QSortFilterProxyModel* model, TableModel* table, const QModelIndex& index) {
	const BaseTreeItem* item = static_cast<BaseTreeItem*>(model->mapToSource(index).internalPointer());
	if (item->baseCategory || item->subCategory) {
		return;
	}

	open_by_id(table, item->id, index.data(Qt::DisplayRole).toString(), index.data(Qt::DecorationRole).value<QIcon>());
}

void ObjectEditor::open_by_id(TableModel* table, const std::string& id, const QString& name, QIcon icon) {
	// If there is already one open for this item
	if (const auto found = dock_manager->findDockWidget(QString::fromStdString(id)); found) {
		found->dockAreaWidget()->setCurrentDockWidget(found);
		found->setFocus();
		found->raise();
		return;
	}

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);

	const auto found = ability_insights.find(id);
	if (found != ability_insights.end()) {
		QLabel* title = new QLabel(QString::fromUtf8((*found)["name"].get<std::string_view>()));
		QFont font1 = title->font();
		font1.setBold(true);
		font1.setPointSize(15);
		title->setFont(font1);

		std::string all_tags = "Tags: ";
		for (const auto& tag : (*found)["tags"]) {
			all_tags += tag.get<std::string_view>() + " ";
		}

		QLabel* tags = new QLabel(QString::fromStdString(all_tags));
		QFont font2 = tags->font();
		font2.setBold(true);
		tags->setFont(font2);

		QLabel* label = new QLabel(QString::fromUtf8((*found)["raw_text"].get<std::string_view>()));
		QLabel* latest_tested_version = new QLabel("Latest tested version: " + QString::fromUtf8((*found)["latest_tested_version"].get<std::string_view>()));
		latest_tested_version->setFont(font2);

		QLabel* link = new QLabel("Fix mistakes or add info directly in <a href=\"https://docs.google.com/document/d/1z17FTnhyfVL87tJgLmwWks3Low6TuQ0tjfKHXBELWpo/edit\">the Google Docs</a>!");
		link->setOpenExternalLinks(true);

		layout->addWidget(title);
		layout->addWidget(tags);
		layout->addWidget(label);
		layout->addWidget(latest_tested_version);
		layout->addWidget(link);
		label->setWordWrap(true);
	}

	QTableView* view = new QTableView;
	TableDelegate* delegate = new TableDelegate;
	view->setItemDelegate(delegate);
	view->horizontalHeader()->hide();
	view->setAlternatingRowColors(true);
	view->setVerticalHeader(new AlterHeader(Qt::Vertical, view));
	view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
	view->verticalHeader()->setMinimumSectionSize(28);
	view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
	view->setIconSize({ 24, 24 });
	view->setWordWrap(true);
	view->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);
	layout->addWidget(view);

	QWidget* container = new QWidget;
	container->setLayout(layout);

	QScrollArea* area = new QScrollArea;
	area->setWidget(container);
	area->setWidgetResizable(true);

	ads::CDockWidget* dock_tab = new ads::CDockWidget(dock_manager, "");
	dock_tab->setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetDeleteOnClose, true);
	dock_tab->setWidget(area);
	// dock_tab->setWidget(view);
	dock_tab->setObjectName(QString::fromStdString(id));
	dock_tab->setWindowTitle(name);
	dock_tab->setIcon(icon);

	SingleModel* single_model = new SingleModel(table, this);
	single_model->setID(id);
	view->setModel(single_model);

	dock_manager->addDockWidget(ads::CenterDockWidgetArea, dock_tab, dock_area);

	// Scroll just past the ability insights
	const int y = view->mapTo(area->widget(), QPoint(0,0)).y();
	area->verticalScrollBar()->setValue(y);
}

void ObjectEditor::addTypeTreeView(BaseTreeModel* treeModel, BaseFilter*& filter, TableModel* table, QTreeView* view, QIcon icon, QString name, Category category) {
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

	connect(view, &QTreeView::customContextMenuRequested, [=, this](const QPoint& pos) {
		QMenu menu;
		QAction* addAction = menu.addAction("Add " + name);
		QAction* removeAction = menu.addAction("Remove " + name);

		if (category == Category::doodad) {
			QAction* convert_to_destructible = menu.addAction("Convert to destructible");

			{
				const auto selection = view->selectionModel()->selectedIndexes();
				if (selection.isEmpty()) {
					convert_to_destructible->setDisabled(true);
				} else {
					const auto treeItem = static_cast<BaseTreeItem*>(filter->mapToSource(selection.front()).internalPointer());
					if (treeItem->id.empty()) {
						convert_to_destructible->setDisabled(true);
					}
				}
			}

			connect(convert_to_destructible, &QAction::triggered, [=, this]() {
				QModelIndexList selection = view->selectionModel()->selectedIndexes();

				for (const auto& i : selection) {
					BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(filter->mapToSource(i).internalPointer());
					if (treeItem->id.empty()) {
						continue;
					}
					std::string new_id;
					destructibles_table->addRow([&] {
						// We pick a barrel (LTbr) for no good reason
						new_id = convert_doodad_to_destructible(treeItem->id, "LTbr");
					});

					const auto index = destructibles_table->rowIDToIndex(new_id);
					const auto index2 = destructibleTreeModel->mapFromSource(index);
					const auto index3 = destructibleTreeFilter->mapFromSource(index2);
					itemClicked(destructibleTreeFilter, destructibles_table, index3);
				}
			});
		}

		QModelIndexList selection = view->selectionModel()->selectedIndexes();
		if (selection.empty()) {
			removeAction->setDisabled(true);
		} else {
			BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(filter->mapToSource(selection.front()).internalPointer());
			if (!table->slk->shadow_data.contains(treeItem->id) || !table->slk->shadow_data.at(treeItem->id).contains("oldid")) {
				removeAction->setDisabled(true);	
			}
		}
		
		connect(addAction, &QAction::triggered, [=, this]() {
			QDialog* selectdialog = new QDialog(this, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
			selectdialog->resize(300, 560);
			selectdialog->setWindowModality(Qt::WindowModality::WindowModal);

			QLineEdit* nameEdit = new QLineEdit;
			nameEdit->setPlaceholderText("New name");
			nameEdit->setReadOnly(true);

			QLineEdit* id = new QLineEdit;
			id->setPlaceholderText("Free ID");
			id->setText(QString::fromStdString(map->get_unique_id(false)));
			id->setFont(QFont("consolas"));

			QHBoxLayout* nameLayout = new QHBoxLayout;
			nameLayout->addWidget(nameEdit, 3);
			nameLayout->addWidget(id, 1);

			BaseFilter* sub_filter = new BaseFilter;
			sub_filter->slk = table->slk;
			sub_filter->setRecursiveFilteringEnabled(true);
			sub_filter->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
			sub_filter->setSourceModel(treeModel);

			QLineEdit* search = new QLineEdit;
			search->setPlaceholderText("Search " + name);

			QTreeView* sub_view = new QTreeView;
			sub_view->setModel(sub_filter);
			sub_view->setUniformRowHeights(true);
			sub_view->header()->hide();
			sub_view->expandAll();

			QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
			connect(buttonBox, &QDialogButtonBox::accepted, selectdialog, &QDialog::accept);
			connect(buttonBox, &QDialogButtonBox::rejected, selectdialog, &QDialog::reject);

			QVBoxLayout* selectlayout = new QVBoxLayout(selectdialog);
			selectlayout->addLayout(nameLayout);
			selectlayout->addWidget(search);
			selectlayout->addWidget(sub_view);
			selectlayout->addWidget(buttonBox);

			connect(search, &QLineEdit::textChanged, [=](const QString& string) {
				sub_filter->setFilterFixedString(string);
				sub_view->expandAll();
			});

			connect(sub_view->selectionModel(), &QItemSelectionModel::currentChanged, [table, sub_filter, filter, id, nameEdit](const QModelIndex& current, const QModelIndex& previous) {
				if (!current.isValid()) {
					return;
				}
				nameEdit->setText(sub_filter->data(current).toString());
				const BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(sub_filter->mapToSource(current).internalPointer());
				if (treeItem->baseCategory || treeItem->subCategory) {
					return;
				}
				id->setText(QString::fromStdString(map->get_unique_id(!islower(treeItem->id.front()))));
			});

			connect(id, &QLineEdit::textChanged, [buttonBox](const QString& text) {
				buttonBox->button(QDialogButtonBox::Ok)->setEnabled(text.size() == 4);
			});

			auto select = [view, table, sub_filter, filter, selectdialog, id, treeModel](const QModelIndex& index) {
				if (id->text().size() != 4) {
					return;
				}

				const BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(sub_filter->mapToSource(index).internalPointer());
				if (treeItem->baseCategory || treeItem->subCategory) {
					return;
				}

				selectdialog->close();
				table->copyRow(treeItem->id, id->text().toStdString());

				QModelIndex new_index = filter->mapFromSource(treeModel->mapFromSource(table->index(table->rowCount() - 1, 0)));
				view->setCurrentIndex(new_index);
				view->scrollTo(new_index, QAbstractItemView::ScrollHint::PositionAtCenter);
			};

			connect(sub_view, &QTreeView::activated, [select](const QModelIndex& index) { select(index); });

			connect(selectdialog, &QDialog::accepted, [sub_view, select]() {
				auto indices = sub_view->selectionModel()->selectedIndexes();
				if (indices.empty()) {
					return;
				}

				select(indices.front());
			});

			connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), selectdialog), &QShortcut::activated, [=]() {
				search->setFocus();
				search->selectAll();
			});

			selectdialog->show();
			search->setFocus();
		});

		connect(removeAction, &QAction::triggered, [=, this]() {
			std::vector<std::string> ids_to_delete;
			for (const auto& i : selection) {
				BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(filter->mapToSource(i).internalPointer());
				ids_to_delete.push_back(treeItem->id);

				// Close any open dock widget
				if (auto found = dock_manager->findDockWidget(QString::fromStdString(treeItem->id)); found) {
					found->closeDockWidget();
				}
			}
			for (const auto& i : ids_to_delete) {
				table->deleteRow(i);
			}
		});

		menu.exec(view->mapToGlobal(pos));
	});

	QLineEdit* search = new QLineEdit;
	search->setObjectName("search");
	search->setPlaceholderText("Search " + name);
	connect(search, &QLineEdit::textChanged, [=](const QString& string) {
		filter->setFilterFixedString(string);
		view->expandAll();
	});

	QToolButton* hideDefault = new QToolButton;
	hideDefault->setIcon(icon);
	hideDefault->setToolTip("Hide default " + name);
	hideDefault->setCheckable(true);
	connect(hideDefault, &QToolButton::toggled, [=](bool checked) {
		filter->setFilterCustom(checked);
		if (!checked) {
			view->expandAll();
		}
	});

	QToolBar* bar = new QToolBar;
	bar->addWidget(search);
	bar->addWidget(hideDefault);

	ads::CDockWidget* tab = new ads::CDockWidget(dock_manager, name);
	tab->setToolBar(bar);
	tab->setWidget(view);
	tab->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	tab->setFeature(ads::CDockWidget::DockWidgetAlwaysCloseAndDelete, false);
	tab->setIcon(icon);
	if (explorer_area == nullptr) {
		explorer_area = dock_manager->addDockWidget(ads::LeftDockWidgetArea, tab);
	} else {
		dock_manager->addDockWidget(ads::CenterDockWidgetArea, tab, explorer_area);
	}
}

void ObjectEditor::select_id(Category category, const std::string& id) const {
	explorer_area->setCurrentIndex(static_cast<int>(category));
	const auto edit = explorer_area->currentDockWidget()->findChild<QLineEdit*>("search");
	edit->clear();

	switch (category) {
		case Category::unit: {
			const auto index = unitTreeFilter->mapFromSource(unitTreeModel->getIdIndex(id));
			unit_explorer->setCurrentIndex(index);
			unit_explorer->scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
			emit unit_explorer->doubleClicked(index);
			break;
		}
		case Category::item: {
			const auto index = itemTreeFilter->mapFromSource(itemTreeModel->getIdIndex(id));
			item_explorer->setCurrentIndex(index);
			item_explorer->scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
			emit item_explorer->doubleClicked(index);
			break;
		}
		case Category::doodad: {
			const auto index = doodadTreeFilter->mapFromSource(doodadTreeModel->getIdIndex(id));
			doodad_explorer->setCurrentIndex(index);
			doodad_explorer->scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
			emit doodad_explorer->doubleClicked(index);
			break;
		}
		case Category::destructible: {
			const auto index = destructibleTreeFilter->mapFromSource(destructibleTreeModel->getIdIndex(id));
			destructible_explorer->setCurrentIndex(index);
			destructible_explorer->scrollTo(index, QAbstractItemView::PositionAtCenter);
			emit destructible_explorer->doubleClicked(index);
			break;
		}
		case Category::ability: {
			const auto index = abilityTreeFilter->mapFromSource(abilityTreeModel->getIdIndex(id));
			ability_explorer->setCurrentIndex(index);
			ability_explorer->scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
			emit ability_explorer->doubleClicked(index);
			break;
		}
		case Category::upgrade: {
			const auto index = upgradeTreeFilter->mapFromSource(upgradeTreeModel->getIdIndex(id));
			upgrade_explorer->setCurrentIndex(index);
			upgrade_explorer->scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
			emit upgrade_explorer->doubleClicked(index);
			break;
		}
		case Category::buff: {
			const auto index = buffTreeFilter->mapFromSource(buffTreeModel->getIdIndex(id));
			buff_explorer->setCurrentIndex(index);
			buff_explorer->scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
			emit buff_explorer->doubleClicked(index);
			break;
		}
	}
}