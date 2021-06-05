#include "BaseTreeModel.h"

BaseTreeItem::BaseTreeItem(BaseTreeItem* parent) : parent(parent) {
	if (parent != nullptr) {
		parent->appendChild(this);
	}
}

BaseTreeItem::~BaseTreeItem() {
	qDeleteAll(children);
}

void BaseTreeItem::appendChild(BaseTreeItem* item) {
	item->parent = this;
	children.append(item);
}

void BaseTreeItem::removeChild(BaseTreeItem* item) {
	item->parent = nullptr;
	children.removeOne(item);
}

int BaseTreeItem::row() const {
	if (parent)
		return parent->children.indexOf(const_cast<BaseTreeItem*>(this));

	return 0;
}

BaseTreeModel::BaseTreeModel(QObject* parent) : QAbstractProxyModel(parent) {
	rootItem = new BaseTreeItem();

	QFileIconProvider icons;
	folderIcon = icons.icon(QFileIconProvider::Folder);
}

BaseTreeModel::~BaseTreeModel() {
	delete rootItem;
}

int BaseTreeModel::rowCount(const QModelIndex& parent) const {
	if (parent.column() > 0) {
		return 0;
	}

	BaseTreeItem* parentItem;

	if (!parent.isValid()) {
		parentItem = rootItem;
	} else {
		parentItem = static_cast<BaseTreeItem*>(parent.internalPointer());
	}

	return parentItem->children.count();
}

int BaseTreeModel::columnCount(const QModelIndex& parent) const {
	return 1;
}

Qt::ItemFlags BaseTreeModel::flags(const QModelIndex& index) const {
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex BaseTreeModel::index(int row, int column, const QModelIndex& parent) const {
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	BaseTreeItem* parentItem;

	if (!parent.isValid()) {
		parentItem = rootItem;
	} else {
		parentItem = static_cast<BaseTreeItem*>(parent.internalPointer());
	}

	BaseTreeItem* childItem = parentItem->children.at(row);
	if (childItem)
		return createIndex(row, column, childItem);
	return QModelIndex();
}

QModelIndex BaseTreeModel::parent(const QModelIndex& index) const {
	if (!index.isValid()) {
		return {};
	}

	BaseTreeItem* childItem = static_cast<BaseTreeItem*>(index.internalPointer());
	BaseTreeItem* parentItem = childItem->parent;

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

QModelIndex BaseTreeModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}

	const std::string id = slk->index_to_row.at(sourceIndex.row());
	const BaseTreeItem* parent_item = items.at(id)->parent;
	const int row = parent_item->children.indexOf(items.at(id));
	return createIndex(row, 0, items.at(id));
}

QModelIndex BaseTreeModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	BaseTreeItem* item = static_cast<BaseTreeItem*>(proxyIndex.internalPointer());

	if (item->baseCategory || item->subCategory) {
		return {};
	}

	if (slk->column_headers.contains("name")) {
		return createIndex(slk->row_headers.at(item->id), slk->column_headers.at("name"), item);
	} else {
		return createIndex(slk->row_headers.at(item->id), slk->column_headers.at("bufftip"), item);
	}

}

void BaseTreeModel::rowsInserted(const QModelIndex& parent, int first, int last) {
	assert(first == last);

	const std::string id = slk->index_to_row.at(first);
	BaseTreeItem* parent_item = getFolderParent(id);

	beginInsertRows(createIndex(parent_item->row(), 0, parent_item), parent_item->children.size(), parent_item->children.size());
	BaseTreeItem* item = new BaseTreeItem(parent_item);
	item->id = id;
	items.emplace(id, item);
	endInsertRows();
}

void BaseTreeModel::rowsRemoved(const QModelIndex& parent, int first, int last) {
	assert(first == last);

	const std::string id = slk->index_to_row.at(first);
	BaseTreeItem* child = items.at(id);

	BaseTreeItem* parent_item = child->parent;
	const int row = parent_item->children.indexOf(child);

	beginRemoveRows(createIndex(parent_item->row(), 0, parent_item), row, row);
	parent_item->removeChild(child);
	items.erase(id);
	delete child;
	endRemoveRows();
}

void BaseTreeModel::sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
	Q_ASSERT(topLeft.isValid());
	Q_ASSERT(topLeft.model() == this->sourceModel());
	Q_ASSERT(bottomRight.isValid());
	Q_ASSERT(bottomRight.model() == this->sourceModel());

	// If the changed field is one of those that determine the item's location in the tree we have to move it
	if (std::find(categoryChangeFields.begin(), categoryChangeFields.end(), slk->index_to_column[topLeft.column()]) != categoryChangeFields.end()) {
		const std::string id = slk->index_to_row.at(topLeft.row());

		BaseTreeItem* parent_item = items.at(id)->parent;
		int row = parent_item->children.indexOf(items.at(id));

		BaseTreeItem* new_parent = getFolderParent(id);
		const QModelIndex source_parent = createIndex(parent_item->row(), 0, parent_item);
		const QModelIndex target_parent = createIndex(new_parent->row(), 0, new_parent);

		beginMoveRows(source_parent, row, row, target_parent, new_parent->children.size());
		BaseTreeItem* child = parent_item->children[row];
		parent_item->removeChild(child);
		new_parent->appendChild(child);
		endMoveRows();
	}

	emit dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), roles);
}


void BaseTreeModel::setSourceModel(QAbstractItemModel* sourceModel) {
	beginResetModel();

	if (this->sourceModel()) {
		disconnect(sourceModel, &QAbstractItemModel::rowsInserted, this, &BaseTreeModel::rowsInserted);
		disconnect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &BaseTreeModel::rowsRemoved);
		disconnect(sourceModel, &QAbstractItemModel::dataChanged, this, &BaseTreeModel::sourceDataChanged);
	}

	QAbstractProxyModel::setSourceModel(sourceModel);

	connect(sourceModel, &QAbstractItemModel::rowsInserted, this, &BaseTreeModel::rowsInserted);
	connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &BaseTreeModel::rowsRemoved);
	connect(sourceModel, &QAbstractItemModel::dataChanged, this, &BaseTreeModel::sourceDataChanged);

	endResetModel();
}

QVariant BaseTreeModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return {};
	}

	BaseTreeItem* item = static_cast<BaseTreeItem*>(index.internalPointer());

	switch (role) {
		case Qt::DecorationRole:
			if (item->baseCategory || item->subCategory) {
				return folderIcon;
			}
			if (slk->column_headers.contains("art")) {
				return sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("art")), role);
			} else {
				return sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("buffart")), role);
			}
		case Qt::TextColorRole:
			if (item->baseCategory || item->subCategory) {
				return {};
			}

			if (slk->shadow_data.contains(item->id)) {
				return QColor("violet");
			} else {
				return {};
			}
		default:
			return {};
	}
}

bool BaseFilter::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
	QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
	BaseTreeItem* item = static_cast<BaseTreeItem*>(index0.internalPointer());

	if (filterCustom) {
		if (item->baseCategory || item->subCategory) {
			return false;
		}

		if (!(slk->shadow_data.contains(item->id) && slk->shadow_data.at(item->id).contains("oldid"))) {
			return false;
		}
	}

	return sourceModel()->data(index0).toString().contains(filterRegExp());
}

void BaseFilter::setFilterCustom(bool filter) {
	filterCustom = filter;
	invalidateFilter();
}