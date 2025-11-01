#pragma once

#include <string>
#include <print>

#include <QEvent>
#include <QListView>
#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QConcatenateTablesProxyModel>

#include "doodad_list_model.h"
#include "destructible_list_model.h"

import UnitListModel;

class GlobalSearchWidget : public QDialog {
	Q_OBJECT

	QLineEdit* edit = new QLineEdit;
	QPushButton* case_sensitive = new QPushButton;
	QPushButton* match_whole_word = new QPushButton;
	QPushButton* regular_expression = new QPushButton;

	DoodadListModel* doodad_list_model;
	DoodadListFilter* doodad_filter_model;
	DestructableListModel* destructable_list_model;
	DestructableListFilter* destructable_filter_model;
	UnitListModel* unit_list_model;
	UnitListFilter* units_filter_model;

	QConcatenateTablesProxyModel* concat_table;


	QListView* list;

public:
	GlobalSearchWidget(QWidget* parent = nullptr);

	void changeEvent(QEvent* e) override {
		if (e->type() == QEvent::ActivationChange && !isActiveWindow()) {
			close();
		}
	}

	signals:
		void text_changed(QString text);
	void previous();
	void next();

};