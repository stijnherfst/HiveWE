#pragma once

#include <QAbstractItemModel>

#include <QVariant>
#include <QVector>
#include <QIcon>
#include <QMenu>
#include <QTreeView>
#include "Triggers.h"

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
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& index) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	void insertItem(const QModelIndex& parent, Classifier classifier, int id);
	void deleteItem(const QModelIndex& item);


private:
	TreeItem* rootItem;

	std::unordered_map<int, TreeItem*> folders;
	std::unordered_map<int, TreeItem*> files;

	QIcon folder_icon;
	QIcon gui_icon;
	QIcon gui_icon_disabled;
	QIcon script_icon;
	QIcon script_icon_disabled;
	QIcon variable_icon;
	QIcon comment_icon;
};

class TriggerExplorer : public QTreeView {
	Q_OBJECT

	QMenu* contextMenu = new QMenu(this);
	QAction* addCategory = new QAction;
	QAction* addGuiTrigger = new QAction;
	QAction* addJassTrigger = new QAction;
	QAction* addComment = new QAction;
	QAction* deleteRow = new QAction;
	QAction* renameRow = new QAction;
	QAction* isEnabled = new QAction;
	QAction* runOnInitialization = new QAction;
	QAction* initiallyOn = new QAction;

public:
	explicit TriggerExplorer(QWidget* parent = nullptr);

	void createCategory();
	void createJassTrigger();
	void createGuiTrigger();
	void createComment();
	void deleteSelection();

signals:
	void itemAboutToBeDeleted(TreeItem* item);
};