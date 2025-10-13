#include <functional>

#include <QStringList>
#include <QFileIconProvider>
#include <QPainter>
#include <QMessageBox>
#include <QStack>
#include <set>

#include "trigger_model.h"

#include "trigger_explorer.h"
#include "HiveWE.h"

import Triggers;
import Utilities;
import MapGlobal;
import OpenGLUtilities;
import Globals;

constexpr int map_header_id = 0;

TriggerExplorer::TriggerExplorer(QWidget* parent) : QTreeView(parent) {
	setSelectionMode(QAbstractItemView::SingleSelection);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setUniformRowHeights(true);
	setHeaderHidden(true);
	setEditTriggers(QAbstractItemView::EditTrigger::EditKeyPressed);
	setDragEnabled(true);
	setDragDropMode(QAbstractItemView::InternalMove);
	setAcceptDrops(true);
	setDropIndicatorShown(true);
	setDefaultDropAction(Qt::MoveAction);

	contextMenu->addAction(addCategory);
	contextMenu->addAction(addGuiTrigger);
	contextMenu->addAction(addJassTrigger);
	contextMenu->addAction(addComment);
	contextMenu->addAction(deleteRow);
	contextMenu->addAction(renameRow);
	contextMenu->addAction(isEnabled);
	contextMenu->addAction(runOnInitialization);
	contextMenu->addAction(initiallyOn);
	isEnabled->setCheckable(true);
	runOnInitialization->setCheckable(true);
	initiallyOn->setCheckable(true);

	addCategory->setText("Add Category");
	addGuiTrigger->setText("Add GUI Trigger");
	addJassTrigger->setText("Add Jass Trigger");
	addComment->setText("Add Comment");
	deleteRow->setText("Delete");
	renameRow->setText("Rename");
	isEnabled->setText("Enabled");
	runOnInitialization->setText("Run on Map Initialization");
	initiallyOn->setText("Initially On");

	connect(this, &QTreeView::customContextMenuRequested, [&](const QPoint& point) {
		const int count = selectionModel()->selectedRows().count();

		if (count == 0) {
			selectionModel()->select(model()->index(0, 0), QItemSelectionModel::SelectionFlag::ClearAndSelect);
		}

		const TreeItem* item = static_cast<TreeItem*>(selectionModel()->selectedRows().front().internalPointer());

		addCategory->setVisible(item->id == 0 || item->type == Classifier::category);
		addGuiTrigger->setVisible(item->type == Classifier::category);
		addJassTrigger->setVisible(item->type == Classifier::category);
		addComment->setVisible(item->type == Classifier::category);
		renameRow->setVisible(item->id != 0);
		deleteRow->setVisible(item->id != 0);

		isEnabled->setVisible(item->id != 0  && (item->type == Classifier::gui || item->type == Classifier::script));
		isEnabled->setChecked(item->enabled);
		initiallyOn->setVisible(item->type == Classifier::gui);
		initiallyOn->setChecked(item->initially_on);

		runOnInitialization->setVisible(false);
		if (item->type == Classifier::script || item->type == Classifier::gui) {
			for (int i = 0; i < map->triggers.triggers.size(); i++) {
				Trigger& trigger = map->triggers.triggers[i];
				if (trigger.id == item->id) {
					runOnInitialization->setVisible(item->type == Classifier::gui && trigger.is_script);
					runOnInitialization->setChecked(item->run_on_initialization);
					break;
				}
			}
		}
 
		contextMenu->popup(viewport()->mapToGlobal(point));
	});

	connect(addCategory, &QAction::triggered, this, &TriggerExplorer::createCategory);
	connect(addGuiTrigger, &QAction::triggered, this, &TriggerExplorer::createGuiTrigger);
	connect(addJassTrigger, &QAction::triggered, this, &TriggerExplorer::createJassTrigger);
	connect(addComment, &QAction::triggered, this, &TriggerExplorer::createComment);
	connect(deleteRow, &QAction::triggered, this, &TriggerExplorer::deleteSelection);
	connect(renameRow, &QAction::triggered, this, [&]() {
		const auto index = selectionModel()->selectedRows().front();

		edit(index);
	});
	connect(isEnabled, &QAction::triggered, this, [&](bool checked) {
		const auto index = selectionModel()->selectedRows().front();
		TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

		item->enabled = checked;

		for (int i = 0; i < map->triggers.triggers.size(); i++) {
			Trigger& trigger = map->triggers.triggers[i];
			if (trigger.id == item->id) {
				trigger.is_enabled = checked;
				break;
			}
		}
	});
	connect(runOnInitialization, &QAction::triggered, this, [&](bool checked) {
		const auto index = selectionModel()->selectedRows().front();
		TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

		item->run_on_initialization = checked;

		for (int i = 0; i < map->triggers.triggers.size(); i++) {
			Trigger& trigger = map->triggers.triggers[i];
			if (trigger.id == item->id) {
				trigger.run_on_initialization = checked;
				break;
			}
		}
	});
	connect(initiallyOn, &QAction::triggered, this, [&](bool checked) {
		auto index = selectionModel()->selectedRows().front();
		TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

		item->initially_on = checked;

		for (int i = 0; i < map->triggers.triggers.size(); i++) {
			Trigger& trigger = map->triggers.triggers[i];
			if (trigger.id == item->id) {
				trigger.initially_on = checked;
				break;
			}
		}
	});
}

void TriggerExplorer::createCategory() {
	QModelIndex index;
	if (selectionModel()->selectedRows().empty()) {
		index = model()->index(0, 0);
	} else {
		index = selectionModel()->selectedRows().front();
	}

	TreeItem* parent_item = static_cast<TreeItem*>(index.internalPointer());

	if (parent_item->type != Classifier::category && parent_item->id != map_header_id) {
		parent_item = parent_item->parent;
		index = index.parent();
	}

	TriggerCategory category;
	category.name = "New Category";
	category.id = ++Trigger::next_id;
	category.parent_id = parent_item->id;
	map->triggers.categories.push_back(category);

	dynamic_cast<TreeModel*>(model())->insertItem(index, Classifier::category, category.id);
	expand(index);


	selectionModel()->setCurrentIndex(index.sibling(parent_item->children.size() - 1, 0), QItemSelectionModel::SelectionFlag::ClearAndSelect);
}

void TriggerExplorer::createJassTrigger() {
	QModelIndex index;
	if (selectionModel()->selectedRows().empty()) {
		index = model()->index(0, 0).sibling(0, 0);
	} else {
		index = selectionModel()->selectedRows().front();
	}

	TreeItem* parent_item = static_cast<TreeItem*>(index.internalPointer());

	// Select a category
	if (parent_item->id == map_header_id) {
		parent_item = parent_item->children.front();
		index = index.sibling(0, 0);
	}

	// Select a category
	if (parent_item->type != Classifier::category) {
		parent_item = parent_item->parent;
		index = index.parent();
	}

	Trigger trigger;
	trigger.classifier = Classifier::script;
	trigger.name = "New Trigger";
	trigger.id = ++Trigger::next_id;
	trigger.is_script = true;
	trigger.parent_id = parent_item->id;
	map->triggers.triggers.push_back(trigger);
	
	dynamic_cast<TreeModel*>(model())->insertItem(index, trigger.classifier, trigger.id);
	expand(index);
	selectionModel()->setCurrentIndex(index.sibling(parent_item->children.size() - 1, 0), QItemSelectionModel::SelectionFlag::ClearAndSelect);
}

void TriggerExplorer::createGuiTrigger() {
	QModelIndex index;
	if (selectionModel()->selectedRows().empty()) {
		index = model()->index(0, 0).sibling(0, 0);
	} else {
		index = selectionModel()->selectedRows().front();
	}

	TreeItem* parent_item = static_cast<TreeItem*>(index.internalPointer());

	if (parent_item->id == map_header_id) {
		parent_item = parent_item->children.front();
		index = index.sibling(0, 0);
	}

	// Select a category
	if (parent_item->type != Classifier::category) {
		parent_item = parent_item->parent;
		index = index.parent();
	}

	Trigger trigger;
	trigger.classifier = Classifier::gui;
	trigger.name = "New Trigger";
	trigger.id = ++Trigger::next_id;
	trigger.parent_id = parent_item->id;
	map->triggers.triggers.push_back(trigger);

	dynamic_cast<TreeModel*>(model())->insertItem(index, trigger.classifier, trigger.id);
	expand(index);
	selectionModel()->setCurrentIndex(index.sibling(parent_item->children.size() - 1, 0), QItemSelectionModel::SelectionFlag::ClearAndSelect);
}

void TriggerExplorer::createVariable() {
	QModelIndex index;
	if (selectionModel()->selectedRows().empty()) {
		index = model()->index(0, 0).sibling(0, 0);
	} else {
		index = selectionModel()->selectedRows().front();
	}

	TreeItem* parent_item = static_cast<TreeItem*>(index.internalPointer());

	if (parent_item->id == map_header_id) {
		parent_item = parent_item->children.front();
		index = index.sibling(0, 0);
	}

	// Select a category
	if (parent_item->type != Classifier::category) {
		parent_item = parent_item->parent;
		index = index.parent();
	}

	TriggerVariable variable;
	variable.id = ++Trigger::next_id;
	variable.name = "NewVariable";
	variable.parent_id = parent_item->id;
	map->triggers.variables.push_back(variable);

	dynamic_cast<TreeModel*>(model())->insertItem(index, Classifier::variable, variable.id);
	expand(index);
	selectionModel()->setCurrentIndex(index.sibling(parent_item->children.size() - 1, 0), QItemSelectionModel::SelectionFlag::ClearAndSelect);
}

void TriggerExplorer::createComment() {
	QModelIndex index;
	if (selectionModel()->selectedRows().empty()) {
		index = model()->index(0, 0).sibling(0, 0);
	} else {
		index = selectionModel()->selectedRows().front();
	}

	TreeItem* parent_item = static_cast<TreeItem*>(index.internalPointer());

	if (parent_item->id == map_header_id) {
		parent_item = parent_item->children.front();
		index = index.sibling(0, 0);
	}

	// Select a category
	if (parent_item->type != Classifier::category) {
		parent_item = parent_item->parent;
		index = index.parent();
	}

	Trigger trigger;
	trigger.classifier = Classifier::comment;
	trigger.name = "New Comment";
	trigger.id = ++Trigger::next_id;
	trigger.parent_id = parent_item->id;
	map->triggers.triggers.push_back(trigger);

	dynamic_cast<TreeModel*>(model())->insertItem(index, trigger.classifier, trigger.id);
	expand(index);
	selectionModel()->setCurrentIndex(index.sibling(parent_item->children.size() - 1, 0), QItemSelectionModel::SelectionFlag::ClearAndSelect);
}

void recursively_delete(TreeItem* parent) {
	for (auto child : parent->children) {
		recursively_delete(child);
	}

	switch (parent->type) {
		case Classifier::comment:
		case Classifier::gui:
		case Classifier::script:
			for (int i = 0; i < map->triggers.triggers.size(); i++) {
				const Trigger& trigger = map->triggers.triggers[i];
				if (trigger.id == parent->id) {
					map->triggers.triggers.erase(map->triggers.triggers.begin() + i);
					break;
				}
			}
			break;
		case Classifier::category:
			for (int i = 0; i < map->triggers.categories.size(); i++) {
				const TriggerCategory& category = map->triggers.categories[i];
				if (category.id == parent->id) {
					map->triggers.categories.erase(map->triggers.categories.begin() + i);
					break;
				}
			}
			break;
		case Classifier::variable:
			for (int i = 0; i < map->triggers.categories.size(); i++) {
				const TriggerVariable& variable = map->triggers.variables[i] ;
				if (variable.id == parent->id) {
					map->triggers.variables.erase(map->triggers.variables.begin() + i);
					break;
				}
			}
			break;
	}
};

void TriggerExplorer::deleteSelection() {
	auto indices = selectionModel()->selectedRows();

	int choice = QMessageBox::question(this, "Delete selected items?", "Are you sure you want to delete these triggers PERMANENTLY?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if (choice == QMessageBox::Yes) {
		for (const auto index : indices) {
			if (index.isValid()) {
				TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
				recursively_delete(item);
				emit itemAboutToBeDeleted(item);

				dynamic_cast<TreeModel*>(model())->deleteItem(index);
			}
		}
	}
}

