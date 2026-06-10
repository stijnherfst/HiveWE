#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QTreeView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

class AssetFilterModel : public QSortFilterProxyModel {
	Q_OBJECT
  public:
	using QSortFilterProxyModel::QSortFilterProxyModel;

	void set_show_used(bool show);
	void set_show_unused(bool show);

  protected:
	bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
	bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

  private:
	bool show_used = true;
	bool show_unused = true;
};

class AssetManager : public QDialog {
	Q_OBJECT
  public:
	explicit AssetManager(QWidget* parent = nullptr);

  private:
	void refresh();
	void update_status() const;
	void update_delete_button() const;
	void set_unused_checked(bool checked) const;
	void delete_checked();
	void show_context_menu(const QPoint& pos);
	void open_in_editor(const QModelIndex& proxy_index) const;
	void remove_object_references(const std::string& id);

	QLineEdit* search_edit;
	QCheckBox* select_all_unused_box;
	QCheckBox* show_used_box;
	QCheckBox* show_unused_box;
	QPushButton* delete_button;
	QTreeView* tree_view;
	QLabel* status_label;
	QStandardItemModel* model;
	AssetFilterModel* filter_model;
};
