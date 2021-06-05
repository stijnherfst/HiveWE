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
	QVector<BaseTreeItem*> children;
	BaseTreeItem* parent = nullptr;

	explicit BaseTreeItem(BaseTreeItem* parentItem = nullptr);
	~BaseTreeItem();

	void appendChild(BaseTreeItem* child);
	void removeChild(BaseTreeItem* child);

	int row() const;

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

	void rowsInserted(const QModelIndex& parent, int first, int last);
	void rowsRemoved(const QModelIndex& parent, int first, int last);
	void sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);



	//BaseTreeItem* newItem(std::string id);
	//void removeItem(std::string id);


	virtual BaseTreeItem* getFolderParent(const std::string& id) const {
		return nullptr;
	};

  public:

	explicit BaseTreeModel(QObject* parent = nullptr);
	~BaseTreeModel();

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;

	void setSourceModel(QAbstractItemModel* sourceModel) override;

	BaseTreeItem* rootItem;
	QIcon folderIcon;

	std::vector<std::string> categoryChangeFields;
	
protected:
	slk::SLK* slk;
	std::unordered_map<std::string, BaseTreeItem*> items;
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