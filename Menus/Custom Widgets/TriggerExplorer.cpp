#include <functional>

#include <QStringList>
#include <QFileIconProvider>
#include <QPainter>
#include <QMessageBox>
#include <set>

#include "Utilities.h"
#include "TriggerExplorer.h"
#include "Triggers.h"
#include "HiveWE.h"

TriggerExplorer::TriggerExplorer(QWidget* parent) : QTreeView(parent) {
	setSelectionMode(QAbstractItemView::SingleSelection);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setUniformRowHeights(true);
	setHeaderHidden(true);
	setEditTriggers(QAbstractItemView::EditTrigger::EditKeyPressed);

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
		int count = selectionModel()->selectedRows().count();

		addCategory->setVisible(count == 1);
		addGuiTrigger->setVisible(count == 1);
		addJassTrigger->setVisible(count == 1);
		addComment->setVisible(count == 1);
		renameRow->setVisible(count == 1);

		if (count > 0) {
			TreeItem* item = static_cast<TreeItem*>(selectionModel()->selectedRows().front().internalPointer());

			isEnabled->setVisible(item->type == Classifier::gui || item->type == Classifier::script);
			isEnabled->setChecked(item->enabled);
			initiallyOn->setVisible(item->type == Classifier::gui);
			initiallyOn->setChecked(item->initially_on);

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
		}
 
		contextMenu->popup(viewport()->mapToGlobal(point));
	});

	connect(addCategory, &QAction::triggered, this, &TriggerExplorer::createCategory);
	connect(addGuiTrigger, &QAction::triggered, this, &TriggerExplorer::createGuiTrigger);
	connect(addJassTrigger, &QAction::triggered, this, &TriggerExplorer::createJassTrigger);
	connect(addComment, &QAction::triggered, this, &TriggerExplorer::createComment);
	connect(deleteRow, &QAction::triggered, this, &TriggerExplorer::deleteSelection);
	connect(renameRow, &QAction::triggered, this, [&]() {
		auto index = selectionModel()->selectedRows().front();

		edit(index);
	});
	connect(isEnabled, &QAction::triggered, this, [&](bool checked) {
		auto index = selectionModel()->selectedRows().front();
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
		auto index = selectionModel()->selectedRows().front();
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
	if (selectionModel()->selectedRows().empty()) {
		return;
	}
	auto index = selectionModel()->selectedRows().front();

	TreeItem* parent_item = static_cast<TreeItem*>(index.internalPointer());

	if (parent_item->type != Classifier::category && parent_item->id != 0) {
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
}

void TriggerExplorer::createJassTrigger() {
	if (selectionModel()->selectedRows().empty()) {
		return;
	}
	auto index = selectionModel()->selectedRows().front();

	TreeItem* parent_item = static_cast<TreeItem*>(index.internalPointer());

	if (parent_item->type != Classifier::category) {
		parent_item = parent_item->parent;
		index = index.parent();
	}

	Trigger trigger;
	trigger.classifier = Classifier::script;
	trigger.name = "New Trigger";
	trigger.id = ++Trigger::next_id;
	trigger.parent_id = parent_item->id;
	map->triggers.triggers.push_back(trigger);
	
	dynamic_cast<TreeModel*>(model())->insertItem(index, trigger.classifier, trigger.id);
	expand(index);
}

void TriggerExplorer::createGuiTrigger() {
	if (selectionModel()->selectedRows().empty()) {
		return;
	}
	auto index = selectionModel()->selectedRows().front();

	TreeItem* parent_item = static_cast<TreeItem*>(index.internalPointer());

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
}

void TriggerExplorer::createComment() {
	if (selectionModel()->selectedRows().empty()) {
		return;
	}
	auto index = selectionModel()->selectedRows().front();
	TreeItem* parent_item = static_cast<TreeItem*>(index.internalPointer());

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

TreeModel::TreeModel(QObject* parent) : QAbstractItemModel(parent) {
	rootItem = new TreeItem();

	TreeItem* map_header = new TreeItem(rootItem);
	map_header->id = 0;
	map_header->type = Classifier::script;
	folders[0] = map_header;

	for (const auto& i : map->triggers.categories) {
		TreeItem* item = new TreeItem(folders[i.parent_id]);
		item->id = i.id;
		item->type = Classifier::category;
		folders[i.id] = item;
	}

	for (auto& i : map->triggers.triggers) {
		TreeItem* item = new TreeItem(folders[i.parent_id]);
		item->id = i.id;
		item->type = i.classifier;
		item->enabled = i.is_enabled;
		item->initially_on = i.initially_on;
		item->run_on_initialization = i.run_on_initialization;
	}

	for (const auto& i : map->triggers.variables) {
		TreeItem* item = new TreeItem(folders[i.parent_id]);
		item->id = i.id;
		item->type = Classifier::variable;
	}

	QFileIconProvider icons;
	folder_icon = icons.icon(QFileIconProvider::Folder);
	gui_icon = icons.icon(QFileIconProvider::File);

	{
		auto pix = gui_icon.pixmap({ 16, 16 });
		QPainter painter(&pix);
		painter.setPen(QPen(QBrush(Qt::red), 2));
		painter.drawLine(4, 4, 12, 12);
		painter.drawLine(12, 4, 4, 12);
		painter.end();

		gui_icon_disabled = QIcon(pix);
	}

	script_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerScript") + ".dds");
	script_icon_disabled = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerScriptDisable") + ".dds");
	variable_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerGlobalVariable") + ".dds");
	comment_icon = texture_to_icon(world_edit_data.data("WorldEditArt", "SEIcon_TriggerComment") + ".dds");
}

TreeModel::~TreeModel() {
	delete rootItem;
}

int TreeModel::columnCount(const QModelIndex& parent) const {
	return 1;
}

void TreeModel::insertItem(const QModelIndex& parent, Classifier classifier, int id) {
	beginInsertRows(parent, rowCount(), rowCount());
	TreeItem* item = static_cast<TreeItem*>(parent.internalPointer());
	
	TreeItem* new_item = new TreeItem(item);
	new_item->type = classifier;
	new_item->id = id;
	endInsertRows();
}

void TreeModel::deleteItem(const QModelIndex& item) {
	beginRemoveRows(item.parent(), item.row(), item.row());
	TreeItem* itemm = static_cast<TreeItem*>(item.internalPointer());
	itemm->parent->removeChild(itemm);
	endRemoveRows();
}

QVariant TreeModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid())
		return QVariant();
	
	TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
	
	switch (role) {
		case Qt::DisplayRole:
		case Qt::EditRole:
			return item->data(index.column());
		case Qt::DecorationRole:
			switch (item->type) {
				case Classifier::category:
					return folder_icon;
				case Classifier::script:
					return item->enabled ? script_icon : script_icon_disabled;
				case Classifier::gui:
					return item->enabled ? gui_icon : gui_icon_disabled;
				case Classifier::variable:
					return variable_icon;
				case Classifier::comment:
					return comment_icon;
			}
			break;
		default:
			return QVariant();
	}	
}

bool TreeModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (!index.isValid())
		return false;

	TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
	return item->setData(index, value, role);
}

Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const {
	if (!index.isValid())
		return Qt::NoItemFlags;

	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
	int role) const {
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return rootItem->data(section);

	return QVariant();
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const {
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	TreeItem* parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<TreeItem*>(parent.internalPointer());

	TreeItem* childItem = parentItem->children.at(row);
	if (childItem)
		return createIndex(row, column, childItem);
	return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex& index) const {
	if (!index.isValid())
		return QModelIndex();

	TreeItem* childItem = static_cast<TreeItem*>(index.internalPointer());
	TreeItem* parentItem = childItem->parent;

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex& parent) const {
	TreeItem* parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<TreeItem*>(parent.internalPointer());

	return parentItem->children.count();
}

TreeItem::TreeItem(TreeItem* parent) : parent(parent) {
	if (parent != nullptr) {
		parent->appendChild(this);
	}
}

TreeItem::~TreeItem() {
	qDeleteAll(children);
}

void TreeItem::appendChild(TreeItem* item) {
	children.append(item);
}

void TreeItem::removeChild(TreeItem* item) {
	children.removeOne(item);
	delete item;
}

QVariant TreeItem::data(int column) const {
	if (column != 0) {
		return QVariant();
	}

	switch (type) {
		case Classifier::comment:
		case Classifier::gui:
		case Classifier::script:
			for (const auto& i : map->triggers.triggers) {
				if (i.id == id) {
					return QString::fromStdString(i.name);
				}
			}
			if (id == 0) {
				return "Map Header";
			}
			return "not found";
			break;
		case Classifier::variable:
			for (const auto& i : map->triggers.variables) {
				if (i.id == id) {
					return QString::fromStdString(i.name);
				}
			}
		case Classifier::category:
			for (const auto& i : map->triggers.categories) {
				if (i.id == id) {
					return QString::fromStdString(i.name);
				}
			}
	}
}

bool TreeItem::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (value.toString().isEmpty()) {
		return false;
	}

	switch (type) {
		case Classifier::comment:
		case Classifier::gui:
		case Classifier::script:
			for (auto& i : map->triggers.triggers) {
				if (i.id == id) {
					i.name = value.toString().toStdString();
					return true;
				}
			}
		case Classifier::variable:
			break;
			for (auto& i : map->triggers.variables) {
				if (i.id == id) {
					i.name = value.toString().toStdString();
					return true;
				}
			}
		case Classifier::category:
			for (auto& i : map->triggers.categories) {
				if (i.id == id) {
					i.name = value.toString().toStdString();
					return true;
				}
			}
			break;

	}
	return false;
}

int TreeItem::row() const {
	if (parent)
		return parent->children.indexOf(const_cast<TreeItem*>(this));

	return 0;
}