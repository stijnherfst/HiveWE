#pragma once

#include <QAbstractItemModel>

#include <QVariant>
#include <QVector>
#include <QIcon>
#include <QMenu>
#include <QTreeView>
#include <QMimeData>

import Triggers;

class TreeItem {
public:
	explicit TreeItem(TreeItem* parentItem = nullptr);
	~TreeItem();

	void appendChild(TreeItem* child);
	void removeChild(TreeItem* child);

	QVariant data(int column) const;
	bool setData(const QModelIndex& index, const QVariant& value, int role);
	int row() const;

	Classifier type = Classifier::category;
	int id = -1;
	bool enabled = true;
	bool run_on_initialization = false;
	bool initially_on = true;
	QVector<TreeItem*> children;
	TreeItem* parent = nullptr;
private:
};

class TreeModel : public QAbstractItemModel {
	Q_OBJECT

public:
	explicit TreeModel(QObject* parent = nullptr);
	~TreeModel();

	QVariant data(const QModelIndex& index, int role) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role) override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	Qt::DropActions supportedDropActions() const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& index) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;


	void insertItem(const QModelIndex& parent, Classifier classifier, int id);
	void deleteItem(const QModelIndex& item);

	bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
	//bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) override;
	bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
	//bool moveRow(const QModelIndex& sourceParent, int sourceRow, const QModelIndex& destinationParent, int destinationChild);

	QStringList mimeTypes() const override;
	QMimeData* mimeData(const QModelIndexList& indexes) const override;
	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

private:
	TreeItem* rootItem;

	std::unordered_map<int, TreeItem*> folders;

	QIcon folder_icon;
	QIcon gui_icon;
	QIcon gui_icon_disabled;
	QIcon script_icon;
	QIcon script_icon_disabled;
	QIcon variable_icon;
	QIcon comment_icon;
};