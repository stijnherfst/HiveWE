#pragma once

#include <string>

#include <QCheckBox>
#include <QDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QTreeView>
#include <QSortFilterProxyModel>

#include "asset_tree_model.h"

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
	void refresh() const;
	void update_status() const;
	void update_delete_button() const;
	void set_unused_checked(bool checked) const;
	void delete_checked();
	void show_context_menu(const QPoint& pos);
	void open_in_editor(const QModelIndex& proxy_index) const;
	void remove_object_references(const std::string& id);
	void show_preview(const QModelIndex& current);
	void show_empty_preview(); // empty GL preview with a "select an asset" message
	void clear_preview();
	void open_selected_in_model_editor();

	QLineEdit* search_edit;
	QCheckBox* select_all_unused_box;
	QCheckBox* show_used_box;
	QCheckBox* show_unused_box;
	QPushButton* delete_button;
	QTreeView* tree_view;
	QLabel* status_label;
	AssetTreeModel* model;
	AssetFilterModel* filter_model;

	QWidget* preview_host;
	QWidget* preview_widget = nullptr;
	QPushButton* open_model_editor_button;
	QString current_model_path; // Relative path of the selected model, empty if selection is not a model
};
