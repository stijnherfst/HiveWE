#pragma once

#include "UnitListModel.h"

#include <QListView>
#include <QComboBox>
#include <QLineEdit>

class UnitSelector : public QWidget {
	Q_OBJECT

public:
	UnitSelector(QWidget* parent = nullptr);

	UnitListModel* list_model;
	UnitListFilter* filter_model;

	QComboBox* race;
	QLineEdit* search;
	QListView* units;

public slots:
	void forceSelection();

signals:
	void unitSelected(std::string id);
};