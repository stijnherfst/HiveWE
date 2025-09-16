#pragma once

#include <QAbstractItemModel>

#include <QVariant>
#include <QVector>
#include <QIcon>
#include <QMenu>
#include <QTreeView>
#include <QMimeData>
#include "trigger_model.h"

import Triggers;

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
	void createVariable();
	void createComment();
	void deleteSelection();

signals:
	void itemAboutToBeDeleted(TreeItem* item);
};