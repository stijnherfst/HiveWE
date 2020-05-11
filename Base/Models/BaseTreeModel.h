#pragma once

#include <QAbstractProxyModel>
#include <QPainter>
#include <QFileIconProvider>

#include <array>
#include <vector>

#include "HiveWE.h"

class BaseTreeItem {
public:
	explicit BaseTreeItem(BaseTreeItem* parentItem = nullptr);
	~BaseTreeItem();

	void appendChild(BaseTreeItem* child);
	void removeChild(BaseTreeItem* child);

	QVariant data() const;
	int row() const;

	QVector<BaseTreeItem*> children;
	BaseTreeItem* parent = nullptr;

	int tableRow = -1;
	bool baseCategory = false;
	bool subCategory = false;
};

class BaseTreeModel : public QAbstractProxyModel {

	void _q_sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;

public:
	void setSourceModel(QAbstractItemModel* sourceModel) override;

	explicit BaseTreeModel(QObject* parent = nullptr);
	~BaseTreeModel();

	BaseTreeItem* rootItem;
	QIcon folderIcon;
};