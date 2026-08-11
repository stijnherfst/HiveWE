#include <filesystem>

#include <QApplication>
#include <QBrush>
#include <QFileIconProvider>
#include <QLocale>
#include <QStyle>

#include "asset_tree_model.h"

import std;
import SLK;
import Globals;
import TableModel;
import ResourceManager;
import QIconResource;
import BinaryReader;
import Hierarchy;
import MDX;
import UnorderedMap;
import Utilities;
import "object_editor/object_editor.h";

namespace fs = std::filesystem;

static constexpr QColor unused_color(200, 120, 0);

// Safety net for pathological dependency chains, the cycle check already handles models referencing each other
static constexpr int max_depth = 8;

struct AssetTreeModel::Node {
	Node* parent = nullptr; // nullptr for top level file items
	int row = 0; // row inside the parent
	int file_row = -1; // index into files, -1 if this user is not a file in the map
	std::string id; // the used_by entry, empty for top level file items
	bool alive = true; // false once the item is removed from the tree, its storage is only reused on a reset
	bool children_built = false;
	std::vector<Node*> children;
};

struct AssetTreeModel::Caches {
	// Shared across files since the same object can use multiple files
	hive::unordered_map<std::string, ResolvedName> name_cache;
	hive::unordered_map<std::string, QIcon> icon_cache;
	// Filled the first time a file's validation column is drawn. nullopt = not a model file
	hive::unordered_map<std::string, std::optional<ValidationSummary>> validation_cache;

	// A deque as the model hands out pointers to these nodes, so they may not move
	std::deque<Node> nodes;
	std::vector<Node*> roots; // one per file, index == row
	hive::unordered_map<std::string, int> path_to_row; // normalized file path -> row in files
};

// Matches the normalization that Map::get_file_usage() applies before storing a file as a user of another file
static std::string normalize_path(const std::string& path) {
	std::string normalized = to_lowercase_copy(path);
	normalize_path_to_forward_slash(normalized);
	// The game first checks .mdx, then .mdl, so both are stored as .mdx
	if (normalized.ends_with(".mdl")) {
		normalized = normalized.substr(0, normalized.size() - 4) + ".mdx";
	}
	return normalized;
}

AssetTreeModel::AssetTreeModel(QObject* parent) : QAbstractItemModel(parent), caches(std::make_unique<Caches>()) {}

AssetTreeModel::~AssetTreeModel() = default;

QIcon get_file_icon(const std::string& path) {
	static const QIcon model_icon = QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView);
	static const QIcon image_icon = QApplication::style()->standardIcon(QStyle::SP_DesktopIcon);
	static const QIcon sound_icon = QApplication::style()->standardIcon(QStyle::SP_MediaVolume);
	static const QIcon file_icon = QFileIconProvider().icon(QFileIconProvider::File);

	std::string ext = fs::path(path).extension().string();
	for (auto& c : ext) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}

	if (ext == ".mdx" || ext == ".mdl") {
		return model_icon;
	}
	if (ext == ".blp" || ext == ".tga" || ext == ".dds" || ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
		return image_icon;
	}
	if (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ogg") {
		return sound_icon;
	}
	return file_icon;
}

QIcon get_object_icon(const TableModel* table, const std::string_view id, const std::string_view art_field) {
	const QVariant v = table->data(id, art_field, Qt::DecorationRole);
	if (v.isValid() && !v.isNull()) {
		return v.value<QIcon>();
	}
	return {};
}

QString get_object_name(const TableModel* table, const std::string_view id, const std::string_view name_field) {
	const QVariant v = table->data(id, name_field, Qt::DisplayRole);
	if (v.isValid() && !v.isNull()) {
		return v.toString();
	}
	return QString::fromStdString(std::string(id));
}

AssetTreeModel::ResolvedName resolve_used_by_name(const std::string& id) {
	if (id == "loadingscreen") {
		return {"Loading Screen", -1};
	}
	if (id == "map script") {
		return {"Map Script", -1};
	}
	if (id == "game override") {
		return {"Overrides a game asset", -1};
	}
	// MDX transitive reference (path contains a slash)
	if (id.contains('/')) {
		return {QString::fromStdString(id), -1};
	}
	const auto display = [&](const QString& name) {
		return name + " (" + QString::fromStdString(id) + ")";
	};
	if (units_slk.row_headers.contains(id)) {
		return {display(get_object_name(units_table, id, "name")), static_cast<int>(ObjectEditor::Category::unit)};
	}
	if (items_slk.row_headers.contains(id)) {
		return {display(get_object_name(items_table, id, "name")), static_cast<int>(ObjectEditor::Category::item)};
	}
	if (abilities_slk.row_headers.contains(id)) {
		return {display(get_object_name(abilities_table, id, "name")), static_cast<int>(ObjectEditor::Category::ability)};
	}
	if (destructibles_slk.row_headers.contains(id)) {
		return {display(get_object_name(destructibles_table, id, "name")), static_cast<int>(ObjectEditor::Category::destructible)};
	}
	if (doodads_slk.row_headers.contains(id)) {
		return {display(get_object_name(doodads_table, id, "name")), static_cast<int>(ObjectEditor::Category::doodad)};
	}
	if (buff_slk.row_headers.contains(id)) {
		QString name = get_object_name(buff_table, id, "editorname");
		if (name.isEmpty() || name == QString::fromStdString(id)) {
			name = get_object_name(buff_table, id, "bufftip");
		}
		return {display(name), static_cast<int>(ObjectEditor::Category::buff)};
	}
	if (upgrade_slk.row_headers.contains(id)) {
		return {display(get_object_name(upgrade_table, id, "name1")), static_cast<int>(ObjectEditor::Category::upgrade)};
	}
	// Fallback, likely a sound name
	return {QString::fromStdString(id), -1};
}

QIcon resolve_used_by_icon(const std::string& id) {
	if (id == "loadingscreen") {
		return QApplication::style()->standardIcon(QStyle::SP_DesktopIcon);
	}
	if (id == "map script") {
		return QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView);
	}
	if (id == "game override") {
		return QApplication::style()->standardIcon(QStyle::SP_DriveHDIcon);
	}
	// MDX transitive reference (path contains a slash)
	if (id.contains('/')) {
		return QFileIconProvider().icon(QFileIconProvider::File);
	}
	// Load the category icon used by DoodadTreeModel / DestructibleTreeModel
	const auto category_icon = [](const std::string& section, char cat) -> QIcon {
		for (const auto& [key, value] : world_edit_data.section(section)) {
			if (!key.empty() && key.front() == cat) {
				return resource_manager.load<QIconResource>(value[1]).value()->icon;
			}
		}
		return {};
	};
	if (units_slk.row_headers.contains(id)) {
		return get_object_icon(units_table, id, "art");
	}
	if (items_slk.row_headers.contains(id)) {
		return get_object_icon(items_table, id, "art");
	}
	if (abilities_slk.row_headers.contains(id)) {
		return get_object_icon(abilities_table, id, "art");
	}
	if (destructibles_slk.row_headers.contains(id)) {
		const std::string_view cat = destructibles_slk.data<std::string_view>("category", id);
		return cat.empty() ? QIcon {} : category_icon("DestructibleCategories", cat.front());
	}
	if (doodads_slk.row_headers.contains(id)) {
		const std::string_view cat = doodads_slk.data<std::string_view>("category", id);
		return cat.empty() ? QIcon {} : category_icon("DoodadCategories", cat.front());
	}
	if (buff_slk.row_headers.contains(id)) {
		return get_object_icon(buff_table, id, "buffart");
	}
	if (upgrade_slk.row_headers.contains(id)) {
		return get_object_icon(upgrade_table, id, "art1");
	}
	// Fallback: likely a sound name
	return QApplication::style()->standardIcon(QStyle::SP_MediaVolume);
}

AssetTreeModel::Node* AssetTreeModel::node_for(const QModelIndex& index) const {
	return index.isValid() ? static_cast<Node*>(index.internalPointer()) : nullptr;
}

QModelIndex AssetTreeModel::index_for(Node* node) const {
	return createIndex(node->row, 0, node);
}

int AssetTreeModel::resolve_file_row(const std::string& id) const {
	const auto found = caches->path_to_row.find(id);
	return found == caches->path_to_row.end() ? -1 : found->second;
}

void AssetTreeModel::detach_children(Node* node) {
	std::vector<Node*> stack = node->children;
	while (!stack.empty()) {
		Node* dead = stack.back();
		stack.pop_back();
		dead->alive = false;
		stack.insert(stack.end(), dead->children.begin(), dead->children.end());
	}
	node->children.clear();
	node->children_built = false;
}

std::vector<AssetTreeModel::Node*> AssetTreeModel::nodes_for_file(const int file_row) const {
	std::vector<Node*> result;
	for (Node& node : caches->nodes) {
		if (node.alive && node.file_row == file_row) {
			result.push_back(&node);
		}
	}
	return result;
}

int AssetTreeModel::child_count(Node* node) const {
	if (node->file_row < 0) {
		return 0;
	}
	// Stop the recursion when this file already appears higher up, otherwise models that reference
	// each other (or themselves) would expand forever
	int depth = 0;
	for (const Node* ancestor = node->parent; ancestor; ancestor = ancestor->parent) {
		if (ancestor->file_row == node->file_row) {
			return 0;
		}
		depth += 1;
	}
	if (depth >= max_depth) {
		return 0;
	}
	return static_cast<int>(files[node->file_row].used_by.size());
}

void AssetTreeModel::ensure_children(Node* node) const {
	if (node->children_built) {
		return;
	}
	node->children_built = true;

	const int count = child_count(node);
	node->children.reserve(count);
	for (int i = 0; i < count; i++) {
		const std::string& id = files[node->file_row].used_by[i];
		Node& child = caches->nodes.emplace_back();
		child.parent = node;
		child.row = i;
		child.file_row = resolve_file_row(id);
		child.id = id;
		node->children.push_back(&child);
	}
}

// Every item carries a pointer to its own node, the nodes of a file item's children are created on demand
QModelIndex AssetTreeModel::index(const int row, const int column, const QModelIndex& parent) const {
	if (!hasIndex(row, column, parent)) {
		return {};
	}
	if (!parent.isValid()) {
		return createIndex(row, column, caches->roots[row]);
	}
	Node* parent_node = node_for(parent);
	ensure_children(parent_node);
	return createIndex(row, column, parent_node->children[row]);
}

QModelIndex AssetTreeModel::parent(const QModelIndex& index) const {
	Node* node = node_for(index);
	if (!node || !node->parent) {
		return {};
	}
	return index_for(node->parent);
}

int AssetTreeModel::rowCount(const QModelIndex& parent) const {
	if (!parent.isValid()) {
		return static_cast<int>(files.size());
	}
	if (parent.column() != 0) {
		return 0;
	}
	return child_count(node_for(parent));
}

int AssetTreeModel::columnCount(const QModelIndex&) const {
	return 4;
}

QVariant AssetTreeModel::data(const QModelIndex& index, const int role) const {
	if (!index.isValid()) {
		return {};
	}

	const Node* item = node_for(index);

	// File item, either top level or a file that uses the file above it
	if (item->file_row >= 0) {
		const FileNode& node = files[item->file_row];
		switch (role) {
			case Qt::DisplayRole:
				switch (index.column()) {
					case 0:
						return QString::fromStdString(node.path);
					case 1:
						return QLocale().formattedDataSize(static_cast<qint64>(node.size));
					case 2:
						return QString::number(node.used_by.size());
					case 3: {
						const auto& v = resolved_validation(node);
						if (!v) {
							return {};
						}
						if (v->parse_error) {
							return "err";
						}
						static constexpr char letters[] = {'E', 'S', 'W', 'U'};
						QStringList tokens;
						for (int i = 0; i < 4; i++) {
							if (v->counts[i] > 0) {
								tokens << QString("%1%2").arg(v->counts[i]).arg(letters[i]);
							}
						}
						return tokens.isEmpty() ? QString("✓") : tokens.join(' ');
					}
				}
				return {};
			case Qt::ToolTipRole:
				if (index.column() == 3) {
					const auto& v = resolved_validation(node);
					if (!v) {
						return {};
					}
					if (v->parse_error) {
						return "Could not read or parse this model.";
					}
					static constexpr const char* names[] = {"Errors", "Severe", "Warnings", "Unused"};
					QStringList lines;
					for (int i = 0; i < 4; i++) {
						lines << QString("%1: %2").arg(names[i]).arg(v->counts[i]);
					}
					// Show the first several messages so the shorthand is actionable
					constexpr int max_messages = 12;
					const int shown = std::min<int>(max_messages, static_cast<int>(v->messages.size()));
					if (shown > 0) {
						lines << QString();
						for (int i = 0; i < shown; i++) {
							lines << QString::fromStdString(v->messages[i]);
						}
						if (static_cast<int>(v->messages.size()) > shown) {
							lines << QString("... and %1 more").arg(v->messages.size() - shown);
						}
					}
					return lines.join('\n');
				}
				return {};
			case Qt::DecorationRole:
				if (index.column() == 0) {
					return get_file_icon(node.path);
				}
				return {};
			case Qt::CheckStateRole:
				// Deleting is done from the top level item only, a file can appear at many places in the tree
				if (index.column() == 0 && !item->parent) {
					return node.check_state;
				}
				return {};
			case Qt::TextAlignmentRole:
				if (index.column() == 1 || index.column() == 2) {
					return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
				}
				return {};
			case Qt::ForegroundRole:
				if (node.used_by.empty()) {
					return QBrush(unused_color);
				}
				return {};
			case IsUnusedRole:
				return node.used_by.empty();
			case SizeRole:
				return node.size;
			case FileRowRole:
				return item->file_row;
			case ValidationSortRole:
				if (index.column() == 3) {
					const auto& v = resolved_validation(node);
					if (!v) {
						return -1;
					}
					if (v->parse_error) {
						return std::numeric_limits<int>::max();
					}
					// Weight by severity so sorting surfaces the worst models first
					return v->counts[0] * 1000000 + v->counts[1] * 10000 + v->counts[2] * 100 + v->counts[3];
				}
				return {};
		}
		return {};
	}

	if (role == FileRowRole) {
		return -1;
	}

	if (index.column() != 0) {
		return {};
	}

	const std::string& id = item->id;
	switch (role) {
		case Qt::DisplayRole:
			return resolved_name(id).display_name;
		case Qt::DecorationRole: {
			const QIcon& icon = resolved_icon(id);
			return icon.isNull() ? QVariant() : icon;
		}
		case ObjectIdRole:
			return QString::fromStdString(id);
		case CategoryRole:
			return resolved_name(id).category;
	}
	return {};
}

bool AssetTreeModel::setData(const QModelIndex& index, const QVariant& value, const int role) {
	if (!index.isValid() || index.column() != 0 || role != Qt::CheckStateRole) {
		return false;
	}
	const Node* item = node_for(index);
	if (item->parent || item->file_row < 0) {
		return false;
	}
	files[item->file_row].check_state = static_cast<Qt::CheckState>(value.toInt());
	emit dataChanged(index, index, {Qt::CheckStateRole});
	return true;
}

QVariant AssetTreeModel::headerData(const int section, const Qt::Orientation orientation, const int role) const {
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
		return {};
	}
	switch (section) {
		case 0:
			return "File";
		case 1:
			return "Size";
		case 2:
			return "Usages";
		case 3:
			return "Validation";
	}
	return {};
}

Qt::ItemFlags AssetTreeModel::flags(const QModelIndex& index) const {
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}
	Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	const Node* item = node_for(index);
	if (!item->parent && index.column() == 0) {
		flags |= Qt::ItemIsUserCheckable;
	}
	return flags;
}

void AssetTreeModel::set_files(std::vector<FileNode>&& new_files) {
	beginResetModel();
	files = std::move(new_files);
	// Sort: unused files first, then alphabetically within each group
	std::ranges::sort(files, [](const FileNode& a, const FileNode& b) {
		const bool a_unused = a.used_by.empty();
		const bool b_unused = b.used_by.empty();
		if (a_unused != b_unused) {
			return a_unused > b_unused;
		}
		return a.path < b.path;
	});
	caches->name_cache.clear();
	caches->icon_cache.clear();
	caches->validation_cache.clear();
	rebuild_nodes();
	endResetModel();
}

void AssetTreeModel::rebuild_nodes() {
	caches->nodes.clear();
	caches->roots.clear();
	caches->path_to_row.clear();

	caches->roots.reserve(files.size());
	for (int row = 0; row < static_cast<int>(files.size()); row++) {
		Node& node = caches->nodes.emplace_back();
		node.row = row;
		node.file_row = row;
		caches->roots.push_back(&node);
		// The first file wins on a collision, which is what Map::get_file_usage() does as well
		caches->path_to_row.emplace(normalize_path(files[row].path), row);
	}
}

void AssetTreeModel::remove_file(const int row) {
	// Everywhere the file is shown as a user of another file it turns into a plain path that can no
	// longer be expanded, so drop the child rows of those items first
	const std::vector<Node*> instances = nodes_for_file(row);
	for (Node* node : instances) {
		if (!node->parent) {
			continue;
		}
		const int count = child_count(node);
		if (count > 0) {
			beginRemoveRows(index_for(node), 0, count - 1);
			detach_children(node);
			node->file_row = -1;
			endRemoveRows();
		} else {
			node->file_row = -1;
		}
	}

	beginRemoveRows(QModelIndex(), row, row);
	files.erase(files.begin() + row);

	Node* removed_root = caches->roots[row];
	detach_children(removed_root);
	removed_root->alive = false;
	removed_root->file_row = -1;
	caches->roots.erase(caches->roots.begin() + row);
	for (int i = row; i < static_cast<int>(caches->roots.size()); i++) {
		caches->roots[i]->row = i;
	}

	for (Node& node : caches->nodes) {
		if (node.file_row > row) {
			node.file_row -= 1;
		}
	}

	caches->path_to_row.clear();
	for (int i = 0; i < static_cast<int>(files.size()); i++) {
		caches->path_to_row.emplace(normalize_path(files[i].path), i);
	}
	endRemoveRows();

	// Their size/usages/validation columns are gone now that they no longer point at a file
	for (Node* node : instances) {
		if (node->alive) {
			emit dataChanged(index_for(node), createIndex(node->row, 3, node));
		}
	}
}

void AssetTreeModel::remove_object_references(const std::string& id) {
	for (int row = 0; row < static_cast<int>(files.size()); row++) {
		FileNode& node = files[row];

		std::vector<int> removals; // descending, so erasing does not shift the entries still to be removed
		for (int child = static_cast<int>(node.used_by.size()) - 1; child >= 0; child--) {
			if (node.used_by[child] == id) {
				removals.push_back(child);
			}
		}
		if (removals.empty()) {
			continue;
		}

		// The file can be shown at several places in the tree and they all show the same children,
		// items where the recursion was cut short show no children at all and are left out
		const std::vector<Node*> instances = nodes_for_file(row);
		std::vector<Node*> expanded;
		for (Node* instance : instances) {
			if (child_count(instance) > 0) {
				expanded.push_back(instance);
			}
		}

		for (const int child : removals) {
			// All items lose the row at the same time, so announce every removal before touching the data
			for (Node* instance : expanded) {
				beginRemoveRows(index_for(instance), child, child);
			}

			node.used_by.erase(node.used_by.begin() + child);
			for (Node* instance : expanded) {
				if (!instance->children_built) {
					continue;
				}
				Node* removed = instance->children[child];
				detach_children(removed);
				removed->alive = false;
				instance->children.erase(instance->children.begin() + child);
				for (int i = child; i < static_cast<int>(instance->children.size()); i++) {
					instance->children[i]->row = i;
				}
			}

			for (size_t i = 0; i < expanded.size(); i++) {
				endRemoveRows();
			}
		}

		// The usage count, unused highlight and IsUnusedRole all derive from used_by
		for (Node* instance : instances) {
			emit dataChanged(index_for(instance), createIndex(instance->row, 3, instance));
		}
	}
}

int AssetTreeModel::file_count() const {
	return static_cast<int>(files.size());
}

const AssetTreeModel::FileNode& AssetTreeModel::file(const int row) const {
	return files[row];
}

void AssetTreeModel::set_check_state(const int row, const Qt::CheckState state) {
	if (files[row].check_state == state) {
		return;
	}
	files[row].check_state = state;
	const QModelIndex idx = index(row, 0);
	emit dataChanged(idx, idx, {Qt::CheckStateRole});
}

const AssetTreeModel::ResolvedName& AssetTreeModel::resolved_name(const std::string& id) const {
	auto found = caches->name_cache.find(id);
	if (found == caches->name_cache.end()) {
		found = caches->name_cache.emplace(id, resolve_used_by_name(id)).first;
	}
	return found->second;
}

const QIcon& AssetTreeModel::resolved_icon(const std::string& id) const {
	auto found = caches->icon_cache.find(id);
	if (found == caches->icon_cache.end()) {
		found = caches->icon_cache.emplace(id, resolve_used_by_icon(id)).first;
	}
	return found->second;
}

const std::optional<AssetTreeModel::ValidationSummary>& AssetTreeModel::resolved_validation(const FileNode& node) const {
	auto found = caches->validation_cache.find(node.path);
	if (found != caches->validation_cache.end()) {
		return found->second;
	}

	std::string ext = fs::path(node.path).extension().string();
	for (auto& c : ext) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	const bool is_mdl = ext == ".mdl";
	if (!is_mdl && ext != ".mdx") {
		// Not a model; cache an empty optional so we don't re-check the extension every draw
		return caches->validation_cache.emplace(node.path, std::nullopt).first->second;
	}

	ValidationSummary summary;

	// Parsing/validation is only done here, the first time the row is drawn, and cached.
	try {
		auto reader_result = hierarchy.open_file(node.path);
		if (!reader_result.has_value()) {
			summary.parse_error = true;
		} else {
			auto file = reader_result.value();
			std::shared_ptr<mdx::MDX> mdx;
			if (is_mdl) {
				const auto view = std::string_view(reinterpret_cast<const char*>(file.buffer.data()), file.buffer.size());
				auto parsed = mdx::MDX::from_mdl(view);
				if (!parsed.has_value()) {
					summary.parse_error = true;
				} else {
					mdx = std::make_shared<mdx::MDX>(std::move(parsed.value()));
				}
			} else {
				mdx = std::make_shared<mdx::MDX>(file);
			}

			if (mdx) {
				for (const auto& message : mdx->validate()) {
					const int severity = static_cast<int>(message.severity);
					if (severity >= 0 && severity < 4) {
						summary.counts[severity]++;
					}
					summary.messages.push_back(message.message);
				}
			}
		}
	} catch (...) {
		summary.parse_error = true;
	}

	return caches->validation_cache.emplace(node.path, std::move(summary)).first->second;
}
