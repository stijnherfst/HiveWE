#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <QAbstractItemModel>
#include <QCheckBox>
#include <QDialog>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QTreeView>
#include <QSortFilterProxyModel>

/// Two-level tree of files and the objects that use them.
class AssetTreeModel : public QAbstractItemModel {
	Q_OBJECT
  public:
	struct FileNode {
		std::string path; // original on-disk relative path (forward slashes, original case)
		uint64_t size = 0;
		Qt::CheckState check_state = Qt::Unchecked;
		std::vector<std::string> used_by; // empty = unused
	};

	using QAbstractItemModel::QAbstractItemModel;

	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& index) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;

	void set_files(std::vector<FileNode>&& new_files);
	void remove_file(int row);
	void remove_object_references(const std::string& id);

	int file_count() const;
	const FileNode& file(int row) const;
	void set_check_state(int row, Qt::CheckState state);

	struct ResolvedName {
		QString display_name;
		int category = -1; // matches ObjectEditor::Category, -1 = not a named object
	};

  private:
	const ResolvedName& resolved_name(const std::string& id) const;
	const QIcon& resolved_icon(const std::string& id) const;

	std::vector<FileNode> files;

	// Shared across files since the same object can use multiple files
	mutable std::unordered_map<std::string, ResolvedName> name_cache;
	mutable std::unordered_map<std::string, QIcon> icon_cache;
};

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
	AssetTreeModel* model;
	AssetFilterModel* filter_model;
};
