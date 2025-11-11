#pragma once

#include <string>
#include <print>

#include <QEvent>
#include <QListView>
#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QConcatenateTablesProxyModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QSortFilterProxyModel>

import DestructibleListModel;
import UnitListModel;
import BaseListModel;
import DoodadListModel;
import ItemListModel;
import AbilityListModel;
import UpgradeListModel;
import BuffListModel;

class ActionListModel : public QAbstractListModel {
public:
	struct Item { QString text; QIcon icon; };
	QList<Item> items;

	explicit ActionListModel(QObject* parent = nullptr) : QAbstractListModel(parent) {
		items = {
			{"Open Object Editor", QIcon("data/icons/ribbon/objecteditor32x32.png")},
			{"Open Model Editor", QIcon("data/icons/ribbon/model_editor.png")},
			{"Open Trigger Editor", QIcon("data/icons/ribbon/triggereditor32x32.png")}
		};
	}

	int rowCount(const QModelIndex& parent = QModelIndex()) const override {
		return parent.isValid() ? 0 : items.size();
	}

	QVariant data(const QModelIndex& index, const int role) const override {
		if (!index.isValid() || index.row() >= items.size()) {
			return {};
		}

		const auto& it = items[index.row()];
		if (role == Qt::DisplayRole) {
			return it.text;
		}
		if (role == Qt::DecorationRole) {
			return it.icon;
		}
		return {};
	}
};

class ExtraTextDelegate : public QStyledItemDelegate {
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	QFont font = QFont("Consolas");

	void paint(QPainter *painter, const QStyleOptionViewItem &option,
			   const QModelIndex &index) const override {

		QStyledItemDelegate::paint(painter, option, index);

		const QString rightText = index.data(Qt::UserRole).toString();
		if (rightText.isEmpty()) {
			return;
		}

		painter->save();
		painter->setFont(font);
		painter->setPen(Qt::gray);

		const QRect rect = option.rect;
		const QRect rightRect = rect.adjusted(6, 0, -6, 0);
		const QFontMetrics fm = painter->fontMetrics();
		const int x = rightRect.right() - fm.horizontalAdvance(rightText);
		const int y = rightRect.top() + (rightRect.height() - fm.height()) / 2;

		painter->drawText(x, y + fm.ascent(), rightText);
		painter->restore();
	}
};

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

	ActionListModel* action_model;
	QSortFilterProxyModel* action_filter_model;

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