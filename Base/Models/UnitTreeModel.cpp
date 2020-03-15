#include "UnitTreeModel.h"


UnitTreeItem::UnitTreeItem(UnitTreeItem* parent) : parent(parent) {
	if (parent != nullptr) {
		parent->appendChild(this);
	}
}

UnitTreeItem::~UnitTreeItem() {
	qDeleteAll(children);
}

void UnitTreeItem::appendChild(UnitTreeItem* item) {
	children.append(item);
}

void UnitTreeItem::removeChild(UnitTreeItem* item) {
	children.removeOne(item);
	delete item;
}

QVariant UnitTreeItem::data() const {
	return "dab";
}

int UnitTreeItem::row() const {
	if (parent)
		return parent->children.indexOf(const_cast<UnitTreeItem*>(this));

	return 0;
}

UnitTreeModel::UnitTreeModel(QObject* parent) : QAbstractProxyModel(parent) {
	rootItem = new UnitTreeItem();

	for (const auto& i : baseCategories) {
		UnitTreeItem* item = new UnitTreeItem(rootItem);
		item->baseCategory = true;
	}

	for (const auto& i : rootItem->children) {
		for (const auto& subCategory : subCategories) {
			UnitTreeItem* item = new UnitTreeItem(i);
			item->subCategory = true;
		}
	}

	for (int i = 1; i < units_slk.rows; i++) {
		std::string race = units_slk.data("race", i);
		if (units_slk.data("race", i) == "human") {
			bool isHostile = units_slk.data("hostilepal", i) == "1";
			bool isBuilding = units_slk.data("isbldg", i) == "1";
			bool isHero = isupper(units_slk.data("unitid", i).front());
			bool isSpecial = units_slk.data("special", i) == "1";

			int subIndex = 0;
			if (isSpecial) {
				subIndex = 3;
			} else if (isBuilding) {
				subIndex = 1;
			} else if (isHero) {
				subIndex = 2;
			}

			UnitTreeItem* item = new UnitTreeItem(rootItem->children[0]->children[subIndex]);
			item->tableRow = i;
		}
	}

	QFileIconProvider icons;
	folderIcon = icons.icon(QFileIconProvider::Folder);
}

UnitTreeModel::~UnitTreeModel() {
	delete rootItem;
}

void UnitTreeModel::_q_sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
	Q_ASSERT(topLeft.isValid() ? topLeft.model() == sourceModel() : true);
	Q_ASSERT(bottomRight.isValid() ? bottomRight.model() == sourceModel() : true);



	UnitTreeItem* item = static_cast<UnitTreeItem*>(topLeft.internalPointer());

	std::cout << "Source model row " << topLeft.row() << " column" << topLeft.column() << "\n";
	std::cout << "Proxy model row " << mapFromSource(topLeft).row() << " column" << mapFromSource(topLeft).column() << "\n";
	emit dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), {});
}

void UnitTreeModel::setSourceModel(QAbstractItemModel* sourceModel) {
	QAbstractProxyModel::setSourceModel(sourceModel);

	connect(sourceModel, &QAbstractItemModel::dataChanged, this, &UnitTreeModel::_q_sourceDataChanged);
}

QModelIndex UnitTreeModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}

	std::string race = units_slk.data("race", sourceIndex.row());
	bool isHostile = units_slk.data("hostilepal", sourceIndex.row()) == "1";
	bool isBuilding = units_slk.data("isbldg", sourceIndex.row()) == "1";
	bool isHero = isupper(units_slk.data("unitid", sourceIndex.row()).front());
	bool isSpecial = units_slk.data("special", sourceIndex.row()) == "1";

	int subIndex = 0;
	if (isSpecial) {
		subIndex = 3;
	} else if (isBuilding) {
		subIndex = 1;
	} else if (isHero) {
		subIndex = 2;
	}

	if (race == "human") {
		auto items = rootItem->children[0]->children[subIndex]->children;
		for (int i = 0; i < items.size(); i++) {
			UnitTreeItem* item = items[i];
			if (item->tableRow == sourceIndex.row()) {
				return createIndex(i, 0, item);
			}
		}
	}

	return {};
}

QModelIndex UnitTreeModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	UnitTreeItem* item = static_cast<UnitTreeItem*>(proxyIndex.internalPointer());
	if (!item->baseCategory && !item->subCategory) {
		return createIndex(item->tableRow, units_slk.header_to_column.at("name"), item);
	}
	return {};
}

QVariant UnitTreeModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return {};
	}

	UnitTreeItem* item = static_cast<UnitTreeItem*>(index.internalPointer());

	switch (role) {
		case Qt::EditRole:
		case Qt::DisplayRole:
			if (item->baseCategory) {
				return QString::fromStdString(baseCategories[index.row()]);
			} else if (item->subCategory) {
				return QString::fromStdString(subCategories[index.row()]);
			} else {
				if (units_slk.data("campaign", item->tableRow) == "1") {
					const std::string properNames = units_slk.data("propernames", item->tableRow);

					if (!properNames.empty()) {
						return QString::fromStdString(properNames).split(',').first();
					}
				}

				return QAbstractProxyModel::data(index, role);
			}
		case Qt::DecorationRole:
			if (item->tableRow < 0) {
				return folderIcon;
			}
			return sourceModel()->data(sourceModel()->index(item->tableRow, units_slk.header_to_column.at("art")), role);
		default:
			return {};
	}
}

int UnitTreeModel::rowCount(const QModelIndex& parent) const {
	if (parent.column() > 0) {
		return 0;
	}

	UnitTreeItem* parentItem;

	if (!parent.isValid()) {
		parentItem = rootItem;
	} else {
		parentItem = static_cast<UnitTreeItem*>(parent.internalPointer());
	}

	return parentItem->children.count();
}

int UnitTreeModel::columnCount(const QModelIndex& parent) const {
	return 1;
}

Qt::ItemFlags UnitTreeModel::flags(const QModelIndex& index) const {
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex UnitTreeModel::index(int row, int column, const QModelIndex& parent) const {
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	UnitTreeItem* parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<UnitTreeItem*>(parent.internalPointer());

	UnitTreeItem* childItem = parentItem->children.at(row);
	if (childItem)
		return createIndex(row, column, childItem);
	return QModelIndex();
}

QModelIndex UnitTreeModel::parent(const QModelIndex& index) const {
	if (!index.isValid())
		return QModelIndex();

	UnitTreeItem* childItem = static_cast<UnitTreeItem*>(index.internalPointer());
	UnitTreeItem* parentItem = childItem->parent;

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}