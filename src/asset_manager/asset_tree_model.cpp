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
import "object_editor/object_editor.h";

namespace fs = std::filesystem;

static constexpr QColor unused_color(200, 120, 0);

struct AssetTreeModel::Caches {
	// Shared across files since the same object can use multiple files
	hive::unordered_map<std::string, ResolvedName> name_cache;
	hive::unordered_map<std::string, QIcon> icon_cache;
	// Filled the first time a file's validation column is drawn. nullopt = not a model file
	hive::unordered_map<std::string, std::optional<ValidationSummary>> validation_cache;
};

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

// File items have internalId 0, child items have the row of their parent + 1
QModelIndex AssetTreeModel::index(const int row, const int column, const QModelIndex& parent) const {
	if (!hasIndex(row, column, parent)) {
		return {};
	}
	if (!parent.isValid()) {
		return createIndex(row, column, quintptr(0));
	}
	return createIndex(row, column, quintptr(parent.row()) + 1);
}

QModelIndex AssetTreeModel::parent(const QModelIndex& index) const {
	if (!index.isValid() || index.internalId() == 0) {
		return {};
	}
	return createIndex(static_cast<int>(index.internalId() - 1), 0, quintptr(0));
}

int AssetTreeModel::rowCount(const QModelIndex& parent) const {
	if (!parent.isValid()) {
		return static_cast<int>(files.size());
	}
	if (parent.internalId() != 0 || parent.column() != 0) {
		return 0;
	}
	return static_cast<int>(files[parent.row()].used_by.size());
}

int AssetTreeModel::columnCount(const QModelIndex&) const {
	return 4;
}

QVariant AssetTreeModel::data(const QModelIndex& index, const int role) const {
	if (!index.isValid()) {
		return {};
	}

	// Root item
	if (index.internalId() == 0) {
		const FileNode& node = files[index.row()];
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
				if (index.column() == 0) {
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

	if (index.column() != 0) {
		return {};
	}

	const FileNode& node = files[index.internalId() - 1];
	const std::string& id = node.used_by[index.row()];
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
	if (!index.isValid() || index.internalId() != 0 || index.column() != 0 || role != Qt::CheckStateRole) {
		return false;
	}
	files[index.row()].check_state = static_cast<Qt::CheckState>(value.toInt());
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
	if (index.internalId() == 0 && index.column() == 0) {
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
	endResetModel();
}

void AssetTreeModel::remove_file(const int row) {
	beginRemoveRows(QModelIndex(), row, row);
	files.erase(files.begin() + row);
	endRemoveRows();
}

void AssetTreeModel::remove_object_references(const std::string& id) {
	for (int row = 0; row < static_cast<int>(files.size()); row++) {
		FileNode& node = files[row];
		const QModelIndex file_index = index(row, 0);

		bool changed = false;
		for (int child = static_cast<int>(node.used_by.size()) - 1; child >= 0; child--) {
			if (node.used_by[child] == id) {
				beginRemoveRows(file_index, child, child);
				node.used_by.erase(node.used_by.begin() + child);
				endRemoveRows();
				changed = true;
			}
		}

		if (changed) {
			// The usage count, unused highlight and IsUnusedRole all derive from used_by
			emit dataChanged(file_index, index(row, 2));
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
