#include "asset_manager.h"

#include <QApplication>
#include <QBrush>
#include <QSizePolicy>
#include <QFileIconProvider>
#include <QHBoxLayout>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

import std;
import SLK;
import Map;
import MapGlobal;
import Globals;
import TableModel;
import ResourceManager;
import QIconResource;
import WindowHandler;
import "object_editor/object_editor.h";

namespace fs = std::filesystem;

// Custom item data roles
static constexpr int IsUnusedRole = Qt::UserRole; // bool, on file items
static constexpr int ObjectIdRole = Qt::UserRole + 1; // QString, on child items
static constexpr int CategoryRole = Qt::UserRole + 2; // int, on child items (-1 = not an object)
static constexpr int SizeRole = Qt::UserRole + 3; // qulonglong (bytes), on size items

static constexpr QColor unused_color(200, 120, 0);

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
	// Fallback: likely a sound name
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
	return 3;
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
				if (index.column() > 0) {
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
	name_cache.clear();
	icon_cache.clear();
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
	auto found = name_cache.find(id);
	if (found == name_cache.end()) {
		found = name_cache.emplace(id, resolve_used_by_name(id)).first;
	}
	return found->second;
}

const QIcon& AssetTreeModel::resolved_icon(const std::string& id) const {
	auto found = icon_cache.find(id);
	if (found == icon_cache.end()) {
		found = icon_cache.emplace(id, resolve_used_by_icon(id)).first;
	}
	return found->second;
}

bool AssetFilterModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
	// Sort the Size column by raw byte count
	if (left.column() == 1) {
		return left.data(SizeRole).toULongLong() < right.data(SizeRole).toULongLong();
	}
	// Sort the Usages column numerically
	if (left.column() == 2) {
		return left.data(Qt::DisplayRole).toInt() < right.data(Qt::DisplayRole).toInt();
	}
	return QSortFilterProxyModel::lessThan(left, right);
}

void AssetFilterModel::set_show_used(const bool show) {
	beginFilterChange();
	show_used = show;
	endFilterChange();
}

void AssetFilterModel::set_show_unused(const bool show) {
	beginFilterChange();
	show_unused = show;
	endFilterChange();
}

bool AssetFilterModel::filterAcceptsRow(const int source_row, const QModelIndex& source_parent) const {
	if (!show_used || !show_unused) {
		// For child rows check the parent file item, since recursive filtering would
		// otherwise resurface a hidden file whose children match the text filter
		const QModelIndex file_index = source_parent.isValid() ? source_parent : sourceModel()->index(source_row, 0, source_parent);
		const bool is_unused = file_index.data(IsUnusedRole).toBool();
		if ((!show_used && !is_unused) || (!show_unused && is_unused)) {
			return false;
		}
	}
	return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

AssetManager::AssetManager(QWidget* parent) : QDialog(parent) {
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle("Asset Manager");
	resize(600, 800);

	auto* layout = new QVBoxLayout(this);

	// Filter bar
	auto* search_bar = new QHBoxLayout;
	search_edit = new QLineEdit(this);
	search_edit->setPlaceholderText("Search files...");
	search_bar->addWidget(search_edit);

	auto* refresh_button = new QPushButton(this);
	refresh_button->setIcon(QIcon("data/icons/asset_manager/refresh.png"));
	refresh_button->setIconSize(QSize(16, 16));
	refresh_button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	search_bar->addWidget(refresh_button);

	auto* info_button = new QLabel(this);
	info_button->setPixmap(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(QSize(16, 16)));
	info_button->setToolTip(
		"Usage count detection is not perfect and can show files as being unused even if they're actually used.\n"
		"Files that override a game asset are detected and shown as \"Overrides a game asset\".\n"
		"It has difficulty detecting files used in the game code if they're not using forward slashes.\n"
		"Be careful deleting them based only on the \"Usages\" number!"
	);
	search_bar->addWidget(info_button);

	layout->addLayout(search_bar);

	auto* action_bar = new QHBoxLayout;
	show_used_box = new QCheckBox("Show Used", this);
	show_used_box->setChecked(true);
	action_bar->addWidget(show_used_box);

	show_unused_box = new QCheckBox("Show Unused", this);
	show_unused_box->setChecked(true);
	action_bar->addWidget(show_unused_box);

	action_bar->addStretch();

	select_all_unused_box = new QCheckBox("Select All Unused", this);
	action_bar->addWidget(select_all_unused_box);

	delete_button = new QPushButton("Delete checked", this);
	delete_button->setIcon(QApplication::style()->standardIcon(QStyle::SP_TrashIcon));
	delete_button->setEnabled(false);
	action_bar->addWidget(delete_button);

	layout->addLayout(action_bar);

	status_label = new QLabel(this);
	layout->addWidget(status_label);

	model = new AssetTreeModel(this);

	filter_model = new AssetFilterModel(this);
	filter_model->setSourceModel(model);
	filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
	filter_model->setRecursiveFilteringEnabled(true);
	filter_model->setFilterKeyColumn(0);

	tree_view = new QTreeView(this);
	tree_view->setModel(filter_model);
	tree_view->setUniformRowHeights(true);
	tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
	tree_view->setSortingEnabled(true);
	tree_view->sortByColumn(2, Qt::AscendingOrder);
	tree_view->header()->setStretchLastSection(false);
	tree_view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	tree_view->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	tree_view->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

	layout->addWidget(tree_view);

	connect(search_edit, &QLineEdit::textChanged, filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(refresh_button, &QPushButton::clicked, this, &AssetManager::refresh);
	connect(select_all_unused_box, &QCheckBox::toggled, this, &AssetManager::set_unused_checked);
	connect(show_used_box, &QCheckBox::toggled, filter_model, &AssetFilterModel::set_show_used);
	connect(show_unused_box, &QCheckBox::toggled, filter_model, &AssetFilterModel::set_show_unused);
	connect(delete_button, &QPushButton::clicked, this, &AssetManager::delete_checked);
	connect(model, &QAbstractItemModel::dataChanged, this, &AssetManager::update_delete_button);

	// When objects are deleted in the Object Editor, remove their references from the tree.
	// We use rowsAboutToBeRemoved so the SLK index_to_row mapping is still intact.
	const auto make_removal_handler = [this](const slk::SLK& slk) {
		return [this, &slk](const QModelIndex&, const int first, const int last) {
			for (int i = first; i <= last; i++) {
				remove_object_references(slk.index_to_row.at(i));
			}
		};
	};
	connect(units_table, &QAbstractItemModel::rowsAboutToBeRemoved, this, make_removal_handler(units_slk));
	connect(items_table, &QAbstractItemModel::rowsAboutToBeRemoved, this, make_removal_handler(items_slk));
	connect(abilities_table, &QAbstractItemModel::rowsAboutToBeRemoved, this, make_removal_handler(abilities_slk));
	connect(doodads_table, &QAbstractItemModel::rowsAboutToBeRemoved, this, make_removal_handler(doodads_slk));
	connect(destructibles_table, &QAbstractItemModel::rowsAboutToBeRemoved, this, make_removal_handler(destructibles_slk));
	connect(buff_table, &QAbstractItemModel::rowsAboutToBeRemoved, this, make_removal_handler(buff_slk));
	connect(tree_view, &QTreeView::customContextMenuRequested, this, &AssetManager::show_context_menu);
	connect(tree_view, &QTreeView::doubleClicked, this, &AssetManager::open_in_editor);

	refresh();
	show();
}

void AssetManager::refresh() {
	{
		const QSignalBlocker blocker(select_all_unused_box);
		select_all_unused_box->setChecked(false);
	}

	auto results = map->get_file_usage();

	std::vector<AssetTreeModel::FileNode> nodes;
	nodes.reserve(results.size());
	for (auto& [path, size, used_by] : results) {
		AssetTreeModel::FileNode node;
		node.path = std::move(path);
		node.size = size;
		node.used_by.assign(used_by.begin(), used_by.end());
		std::ranges::sort(node.used_by);
		nodes.push_back(std::move(node));
	}
	model->set_files(std::move(nodes));

	update_status();
	update_delete_button();
}

void AssetManager::update_status() const {
	const int total = model->file_count();
	int unused = 0;
	qulonglong savings = 0;
	for (int i = 0; i < total; i++) {
		const auto& file = model->file(i);
		if (file.used_by.empty()) {
			unused += 1;
			savings += file.size;
		}
	}
	status_label->setText(QString("%1 unused · %2 total · %3 can be saved by deleting unused files")
							  .arg(unused)
							  .arg(total)
							  .arg(locale().formattedDataSize(static_cast<qint64>(savings))));
}

void AssetManager::update_delete_button() const {
	int count = 0;
	qulonglong total_size = 0;
	for (int i = 0; i < model->file_count(); i++) {
		const auto& file = model->file(i);
		if (file.check_state == Qt::Checked) {
			count += 1;
			total_size += file.size;
		}
	}
	delete_button->setText(
		count > 0 ? QString("Delete checked (%1, %2)").arg(count).arg(locale().formattedDataSize(static_cast<qint64>(total_size)))
				  : "Delete checked"
	);
	delete_button->setEnabled(count > 0);
}

void AssetManager::set_unused_checked(const bool checked) const {
	// Only affect unused rows that pass the current filters
	for (int i = 0; i < filter_model->rowCount(); i++) {
		const QModelIndex source_index = filter_model->mapToSource(filter_model->index(i, 0));
		if (model->file(source_index.row()).used_by.empty()) {
			model->set_check_state(source_index.row(), checked ? Qt::Checked : Qt::Unchecked);
		}
	}
}

void AssetManager::delete_checked() {
	std::vector<int> checked;
	uint64_t total_size = 0;
	bool any_used = false;
	for (int i = 0; i < model->file_count(); i++) {
		const auto& file = model->file(i);
		if (file.check_state == Qt::Checked) {
			checked.push_back(i);
			total_size += file.size;
			any_used |= !file.used_by.empty();
		}
	}

	if (checked.empty()) {
		return;
	}

	QString message =
		QString("Delete %1 file(s) (%2)?").arg(checked.size()).arg(locale().formattedDataSize(static_cast<int64_t>(total_size)));
	message += "\n\nAre you sure? Detecting unused resources can be inaccurate.";
	if (any_used) {
		message += "\nWarning: some of the checked files are in use or override a game asset!";
	}
	const int answer = QMessageBox::question(this, "Delete files", message, QMessageBox::Yes | QMessageBox::No);
	if (answer != QMessageBox::Yes) {
		return;
	}

	QStringList failures;
	// Iterate in reverse so row removals don't shift the rows of items still to be removed
	for (const int row : checked | std::views::reverse) {
		const QString path_str = QString::fromStdString(model->file(row).path);
		std::error_code ec;
		fs::remove(map->filesystem_path / path_str.toStdString(), ec);
		if (ec) {
			failures.append(QString("%1: %2").arg(path_str, QString::fromStdString(ec.message())));
			continue;
		}
		model->remove_file(row);
	}

	if (!failures.empty()) {
		QMessageBox::warning(this, "Delete failed", "Could not delete the following files:\n" + failures.join('\n'));
	}

	update_status();
	update_delete_button();
}

void AssetManager::remove_object_references(const std::string& id) {
	model->remove_object_references(id);
	update_status();
}

void AssetManager::open_in_editor(const QModelIndex& proxy_index) const {
	if (!proxy_index.isValid()) {
		return;
	}
	const QModelIndex source_index = filter_model->mapToSource(proxy_index).siblingAtColumn(0);
	if (!source_index.parent().isValid()) {
		return; // root (file) item — nothing to open
	}
	const int category = source_index.data(CategoryRole).toInt();
	if (category < 0) {
		return;
	}
	const QString id = source_index.data(ObjectIdRole).toString();
	bool created = false;
	const auto* editor = window_handler.create_or_raise<ObjectEditor>(nullptr, created);
	editor->select_id(static_cast<ObjectEditor::Category>(category), id.toStdString());
}

void AssetManager::show_context_menu(const QPoint& pos) {
	const QModelIndex proxy_index = tree_view->indexAt(pos);
	if (!proxy_index.isValid()) {
		return;
	}

	// Always work with column 0 so IsUnusedRole / ObjectIdRole are accessible
	const QModelIndex source_index = filter_model->mapToSource(proxy_index).siblingAtColumn(0);

	QMenu menu;

	const bool is_root = !source_index.parent().isValid();
	if (is_root) {
		if (source_index.data(IsUnusedRole).toBool()) {
			QAction* delete_action = menu.addAction(QApplication::style()->standardIcon(QStyle::SP_TrashIcon), "Delete file");
			connect(delete_action, &QAction::triggered, [this, row = source_index.row()]() {
				const QString path_str = QString::fromStdString(model->file(row).path);
				const int answer =
					QMessageBox::question(this, "Delete file", QString("Delete '%1'?").arg(path_str), QMessageBox::Yes | QMessageBox::No);
				if (answer != QMessageBox::Yes) {
					return;
				}
				const fs::path full_path = map->filesystem_path / path_str.toStdString();
				std::error_code ec;
				fs::remove(full_path, ec);
				if (ec) {
					QMessageBox::warning(
						this,
						"Delete failed",
						QString("Could not delete '%1':\n%2").arg(path_str, QString::fromStdString(ec.message()))
					);
					return;
				}
				model->remove_file(row);
				update_status();
				update_delete_button();
			});
		}
	} else {
		const int category = source_index.data(CategoryRole).toInt();
		if (category >= 0) {
			QAction* open_action = menu.addAction("Open in Object Editor");
			connect(open_action, &QAction::triggered, [this, proxy_index]() {
				open_in_editor(proxy_index);
			});
		}
	}

	if (!menu.isEmpty()) {
		menu.exec(tree_view->viewport()->mapToGlobal(pos));
	}
}
