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
	children.append(item);
}

void BaseTreeItem::removeChild(BaseTreeItem* item) {
	children.removeOne(item);
	delete item;
}

QVariant BaseTreeItem::data() const {
	return "dab";
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

void BaseTreeModel::_q_sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
	Q_ASSERT(topLeft.isValid() ? topLeft.model() == sourceModel() : true);
	Q_ASSERT(bottomRight.isValid() ? bottomRight.model() == sourceModel() : true);

	//BaseTreeItem* item = static_cast<BaseTreeItem*>(topLeft.internalPointer());

	//std::cout << "Source model row " << topLeft.row() << " column" << topLeft.column() << "\n";
	//std::cout << "Proxy model row " << mapFromSource(topLeft).row() << " column" << mapFromSource(topLeft).column() << "\n";
	emit dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), {});
}

void BaseTreeModel::setSourceModel(QAbstractItemModel* sourceModel) {
	QAbstractProxyModel::setSourceModel(sourceModel);

	connect(sourceModel, &QAbstractItemModel::dataChanged, this, &BaseTreeModel::_q_sourceDataChanged);
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

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<BaseTreeItem*>(parent.internalPointer());

	BaseTreeItem* childItem = parentItem->children.at(row);
	if (childItem)
		return createIndex(row, column, childItem);
	return QModelIndex();
}

QModelIndex BaseTreeModel::parent(const QModelIndex& index) const {
	if (!index.isValid())
		return QModelIndex();

	BaseTreeItem* childItem = static_cast<BaseTreeItem*>(index.internalPointer());
	BaseTreeItem* parentItem = childItem->parent;

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}