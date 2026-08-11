#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <QAbstractItemModel>
#include <QIcon>

// Custom item data roles shared by the model, its proxy and the Asset Manager view
inline constexpr int IsUnusedRole = Qt::UserRole; // bool, on file items
inline constexpr int ObjectIdRole = Qt::UserRole + 1; // QString, on child items
inline constexpr int CategoryRole = Qt::UserRole + 2; // int, on child items (-1 = not an object)
inline constexpr int SizeRole = Qt::UserRole + 3; // qulonglong (bytes), on size items
inline constexpr int ValidationSortRole = Qt::UserRole + 4; // int severity score, on validation items
inline constexpr int FileRowRole = Qt::UserRole + 5; // int row into the file list, -1 if the item is not a file

/// Tree of files and the objects/files that use them. A user that is itself a file in the map can be
/// expanded again to show its own users, so a dependency chain can be followed all the way up.
class AssetTreeModel : public QAbstractItemModel {
	Q_OBJECT
  public:
	struct FileNode {
		std::string path; // original on-disk relative path (forward slashes, original case)
		uint64_t size = 0;
		Qt::CheckState check_state = Qt::Unchecked;
		std::vector<std::string> used_by; // empty = unused
	};

	explicit AssetTreeModel(QObject* parent = nullptr);
	~AssetTreeModel() override;

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

	// Validation result for a .mdx/.mdl file, indexed by mdx::ValidationSeverity
	struct ValidationSummary {
		std::array<int, 4> counts{}; // error, severe, warning, unused
		bool parse_error = false; // file could not be read/parsed
		std::vector<std::string> messages; // full messages for the tooltip
	};

  private:
	const ResolvedName& resolved_name(const std::string& id) const;
	const QIcon& resolved_icon(const std::string& id) const;
	// nullopt means the file is not a model (.mdx/.mdl) and has no validation
	const std::optional<ValidationSummary>& resolved_validation(const FileNode& node) const;

	// One tree item. Top level items are files, deeper items are the ids/files that use their parent.
	struct Node;

	Node* node_for(const QModelIndex& index) const; // nullptr for the invisible root
	QModelIndex index_for(Node* node) const;
	// Removes the children of a node and marks them (and their descendants) as no longer part of the tree
	static void detach_children(Node* node);
	std::vector<Node*> nodes_for_file(int file_row) const; // every place in the tree where this file is shown
	void rebuild_nodes(); // recreates the top level items and the path lookup from the file list
	void ensure_children(Node* node) const; // creates the child nodes of a file item on demand
	int child_count(Node* node) const;
	int resolve_file_row(const std::string& id) const; // -1 if the id is not a file in the map

	std::vector<FileNode> files;

	// Modules import mess with std includes, so keep the caches stuff in a .cpp file
	struct Caches;
	std::unique_ptr<Caches> caches;
};
