#pragma once

#include <string>
#include <print>

#include <QEvent>
#include <QListView>
#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QConcatenateTablesProxyModel>

import DestructibleListModel;
import UnitListModel;
import BaseListModel;
import DoodadListModel;
import ItemListModel;
import AbilityListModel;
import UpgradeListModel;
import BuffListModel;

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
	AbilityListModel* ability_list_model;
	AbilityListFilter* ability_filter_model;
	ItemListModel* items_list_model;
	ItemListFilter* item_filter_model;
	UpgradeListModel* upgrade_list_model;
	UpgradeListFilter* upgrade_filter_model;
	BuffListModel* buff_list_model;
	BuffListFilter* buff_filter_model;


	QConcatenateTablesProxyModel* concat_table;


	QListView* list;

public:
	GlobalSearchWidget(QWidget* parent = nullptr);

	void changeEvent(QEvent* e) override {
		if (e->type() == QEvent::ActivationChange && !isActiveWindow()) {
			close();
		}
	}

	bool eventFilter(QObject *object, QEvent *event) override;

	signals:
		void text_changed(QString text);
	void previous();
	void next();

};