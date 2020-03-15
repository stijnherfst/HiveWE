#pragma once

#include <QAbstractProxyModel>
#include <QPainter>
#include <QFileIconProvider>

#include <array>
#include <vector>

#include "HiveWE.h"

class UnitTreeItem {
public:
	explicit UnitTreeItem(UnitTreeItem* parentItem = nullptr);
	~UnitTreeItem();

	void appendChild(UnitTreeItem* child);
	void removeChild(UnitTreeItem* child);

	QVariant data() const;
	int row() const;

	QVector<UnitTreeItem*> children;
	UnitTreeItem* parent = nullptr;

	int tableRow = -1;
	bool baseCategory = false;
	bool subCategory = false;
};

class UnitTreeModel : public QAbstractProxyModel {
	UnitTreeItem* rootItem;
	QIcon folderIcon;

	std::array<std::string, 7> baseCategories = {
		"Human",
		"Orc",
		"Night Elf",
		"Undead",
		"Neutral - Naga",
		"Neutral Hostile",
		"Neutral Passive"
	};

	std::array<std::string, 4> subCategories = {
		"Units",
		"Buildings",
		"Heroes",
		"Special",
	};

	void _q_sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);


	QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;

	//QVariant headerData(int section, Qt::Orientation orientation, int role) const override;


	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;

public:
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	void setSourceModel(QAbstractItemModel* sourceModel) override;

	explicit UnitTreeModel(QObject* parent = nullptr);
	~UnitTreeModel();
};