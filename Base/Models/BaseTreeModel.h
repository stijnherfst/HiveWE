#pragma once

#include <QAbstractProxyModel>
#include <QPainter>
#include <QFileIconProvider>
#include <QSortFilterProxyModel>
#include <QIdentityProxyModel>

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

	std::string id;
	bool baseCategory = false;
	bool subCategory = false;
};

class BaseTreeModel : public QAbstractProxyModel {
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;

	QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;

	virtual void rowsInserted(const QModelIndex& parent, int first, int last);
	virtual void rowsRemoved(const QModelIndex& parent, int first, int last);

	virtual BaseTreeItem* getFolderParent(const std::string& id) const {
		return nullptr;
	};


  public:

	explicit BaseTreeModel(QObject* parent = nullptr);
	~BaseTreeModel();

	void setSourceModel(QAbstractItemModel* sourceModel) override;

	BaseTreeItem* rootItem;
	QIcon folderIcon;
	
protected:
	slk::SLK* slk;
};


class BaseFilter : public QSortFilterProxyModel {
	Q_OBJECT

	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

	bool filterCustom = false;

 public:
	slk::SLK* slk;

	using QSortFilterProxyModel::QSortFilterProxyModel;

public slots:
	void setFilterCustom(bool filter);
};