#include "asset_manager.h"

#include <QApplication>
#include <QSizePolicy>
#include <QFileIconProvider>
#include <QHBoxLayout>
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

struct ObjectInfo {
	QString display_name;
	QIcon icon;
	int category = -1; // matches ObjectEditor::Category, -1 = not a named object
};

ObjectInfo resolve_used_by_id(const std::string& id) {
	if (id == "loadingscreen") {
		return {"Loading Screen", QApplication::style()->standardIcon(QStyle::SP_DesktopIcon), -1};
	}
	if (id == "map script") {
		return {"Map Script", QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView), -1};
	}
	if (id == "game override") {
		return {"Overrides a game asset", QApplication::style()->standardIcon(QStyle::SP_DriveHDIcon), -1};
	}
	// MDX transitive reference (path contains a slash)
	if (id.contains('/')) {
		return {QString::fromStdString(id), QFileIconProvider().icon(QFileIconProvider::File), -1};
	}
	const auto display = [&](const QString& name) {
		return name + " (" + QString::fromStdString(id) + ")";
	};
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
		return {
			display(get_object_name(units_table, id, "name")),
			get_object_icon(units_table, id, "art"),
			static_cast<int>(ObjectEditor::Category::unit)
		};
	}
	if (items_slk.row_headers.contains(id)) {
		return {
			display(get_object_name(items_table, id, "name")),
			get_object_icon(items_table, id, "art"),
			static_cast<int>(ObjectEditor::Category::item)
		};
	}
	if (abilities_slk.row_headers.contains(id)) {
		return {
			display(get_object_name(abilities_table, id, "name")),
			get_object_icon(abilities_table, id, "art"),
			static_cast<int>(ObjectEditor::Category::ability)
		};
	}
	if (destructibles_slk.row_headers.contains(id)) {
		const std::string_view cat = destructibles_slk.data<std::string_view>("category", id);
		return {
			display(get_object_name(destructibles_table, id, "name")),
			cat.empty() ? QIcon {} : category_icon("DestructibleCategories", cat.front()),
			static_cast<int>(ObjectEditor::Category::destructible)
		};
	}
	if (doodads_slk.row_headers.contains(id)) {
		const std::string_view cat = doodads_slk.data<std::string_view>("category", id);
		return {
			display(get_object_name(doodads_table, id, "name")),
			cat.empty() ? QIcon {} : category_icon("DoodadCategories", cat.front()),
			static_cast<int>(ObjectEditor::Category::doodad)
		};
	}
	if (buff_slk.row_headers.contains(id)) {
		QString name = get_object_name(buff_table, id, "editorname");
		if (name.isEmpty() || name == QString::fromStdString(id)) {
			name = get_object_name(buff_table, id, "bufftip");
		}
		return {display(name), get_object_icon(buff_table, id, "buffart"), static_cast<int>(ObjectEditor::Category::buff)};
	}
	if (upgrade_slk.row_headers.contains(id)) {
		return {
			display(get_object_name(upgrade_table, id, "name1")),
			get_object_icon(upgrade_table, id, "art1"),
			static_cast<int>(ObjectEditor::Category::upgrade)
		};
	}
	// Fallback: likely a sound name
	return {QString::fromStdString(id), QApplication::style()->standardIcon(QStyle::SP_MediaVolume), -1};
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

	model = new QStandardItemModel(this);
	model->setHorizontalHeaderLabels({"File"});

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

	layout->addWidget(tree_view);

	connect(search_edit, &QLineEdit::textChanged, filter_model, &QSortFilterProxyModel::setFilterFixedString);
	connect(refresh_button, &QPushButton::clicked, this, &AssetManager::refresh);
	connect(select_all_unused_box, &QCheckBox::toggled, this, &AssetManager::set_unused_checked);
	connect(show_used_box, &QCheckBox::toggled, filter_model, &AssetFilterModel::set_show_used);
	connect(show_unused_box, &QCheckBox::toggled, filter_model, &AssetFilterModel::set_show_unused);
	connect(delete_button, &QPushButton::clicked, this, &AssetManager::delete_checked);
	connect(model, &QStandardItemModel::itemChanged, this, &AssetManager::update_delete_button);

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
	model->clear();
	model->setHorizontalHeaderLabels({"File", "Size", "Usages"});
	tree_view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	tree_view->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	tree_view->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

	{
		const QSignalBlocker blocker(select_all_unused_box);
		select_all_unused_box->setChecked(false);
	}

	auto results = map->get_file_usage();

	// Sort: unused files first, then alphabetically within each group
	std::ranges::sort(results, [](const FileUsage& a, const FileUsage& b) {
		const bool a_unused = a.used_by.empty();
		const bool b_unused = b.used_by.empty();
		if (a_unused != b_unused) {
			return a_unused > b_unused;
		}
		return a.path < b.path;
	});

	for (const auto& [path, size, used_by] : results) {
		const bool is_unused = used_by.empty();

		auto* file_item = new QStandardItem(QString::fromStdString(path));
		file_item->setEditable(false);
		file_item->setCheckable(true);
		file_item->setData(is_unused, IsUnusedRole);
		file_item->setIcon(get_file_icon(path));

		auto* size_item = new QStandardItem(locale().formattedDataSize(static_cast<qint64>(size)));
		size_item->setEditable(false);
		size_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		size_item->setData(size, SizeRole);

		auto* count_item = new QStandardItem(QString::number(used_by.size()));
		count_item->setEditable(false);
		count_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

		if (is_unused) {
			constexpr QColor orange(200, 120, 0);
			file_item->setForeground(orange);
			size_item->setForeground(orange);
			count_item->setForeground(orange);
		}

		for (const auto& id : used_by) {
			const auto& info = resolve_used_by_id(id);

			auto* child_item = new QStandardItem(info.display_name);
			child_item->setEditable(false);
			if (!info.icon.isNull()) {
				child_item->setIcon(info.icon);
			}
			child_item->setData(QString::fromStdString(id), ObjectIdRole);
			child_item->setData(info.category, CategoryRole);

			file_item->appendRow(child_item);
		}

		model->appendRow({file_item, size_item, count_item});
	}

	update_status();
	update_delete_button();
}

void AssetManager::update_status() const {
	const size_t total = static_cast<size_t>(model->rowCount());
	size_t unused = 0;
	qulonglong savings = 0;
	for (int i = 0; i < model->rowCount(); i++) {
		if (model->item(i)->data(IsUnusedRole).toBool()) {
			unused += 1;
			savings += model->item(i, 1)->data(SizeRole).toULongLong();
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
	for (int i = 0; i < model->rowCount(); i++) {
		if (model->item(i)->checkState() == Qt::Checked) {
			count += 1;
			total_size += model->item(i, 1)->data(SizeRole).toULongLong();
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
		QStandardItem* item = model->itemFromIndex(source_index);
		if (item && item->data(IsUnusedRole).toBool()) {
			item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
		}
	}
}

void AssetManager::delete_checked() {
	std::vector<QStandardItem*> checked;
	qulonglong total_size = 0;
	bool any_used = false;
	for (int i = 0; i < model->rowCount(); i++) {
		QStandardItem* item = model->item(i);
		if (item->checkState() == Qt::Checked) {
			checked.push_back(item);
			total_size += model->item(i, 1)->data(SizeRole).toULongLong();
			any_used |= !item->data(IsUnusedRole).toBool();
		}
	}

	if (checked.empty()) {
		return;
	}

	QString message =
		QString("Delete %1 file(s) (%2)?").arg(checked.size()).arg(locale().formattedDataSize(static_cast<qint64>(total_size)));
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
	for (const auto& item : checked | std::views::reverse) {
		const QString path_str = item->text();
		std::error_code ec;
		fs::remove(map->filesystem_path / path_str.toStdString(), ec);
		if (ec) {
			failures.append(QString("%1: %2").arg(path_str, QString::fromStdString(ec.message())));
			continue;
		}
		model->removeRow(item->row());
	}

	if (!failures.empty()) {
		QMessageBox::warning(this, "Delete failed", "Could not delete the following files:\n" + failures.join('\n'));
	}

	update_status();
	update_delete_button();
}

void AssetManager::remove_object_references(const std::string& id) {
	const QString qid = QString::fromStdString(id);
	constexpr QColor orange(200, 120, 0);

	for (int row = 0; row < model->rowCount(); row++) {
		QStandardItem* const file_item = model->item(row, 0);
		if (!file_item) {
			continue;
		}

		bool changed = false;
		for (int child_row = file_item->rowCount() - 1; child_row >= 0; child_row--) {
			const QStandardItem* const child = file_item->child(child_row);
			if (child && child->data(ObjectIdRole).toString() == qid) {
				file_item->removeRow(child_row);
				changed = true;
			}
		}

		if (!changed) {
			continue;
		}

		const int new_count = file_item->rowCount();
		const bool is_now_unused = (new_count == 0);

		if (QStandardItem* size_item = model->item(row, 1)) {
			size_item->setData(is_now_unused ? QVariant(QBrush(orange)) : QVariant(), Qt::ForegroundRole);
		}
		if (QStandardItem* count_item = model->item(row, 2)) {
			count_item->setText(QString::number(new_count));
			count_item->setData(is_now_unused ? QVariant(QBrush(orange)) : QVariant(), Qt::ForegroundRole);
		}
		file_item->setData(is_now_unused, IsUnusedRole);
		file_item->setData(is_now_unused ? QVariant(QBrush(orange)) : QVariant(), Qt::ForegroundRole);
	}

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
	QStandardItem* item = model->itemFromIndex(source_index);
	if (!item) {
		return;
	}
	const int category = item->data(CategoryRole).toInt();
	if (category < 0) {
		return;
	}
	const QString id = item->data(ObjectIdRole).toString();
	bool created = false;
	auto* editor = window_handler.create_or_raise<ObjectEditor>(nullptr, created);
	editor->select_id(static_cast<ObjectEditor::Category>(category), id.toStdString());
}

void AssetManager::show_context_menu(const QPoint& pos) {
	const QModelIndex proxy_index = tree_view->indexAt(pos);
	if (!proxy_index.isValid()) {
		return;
	}

	// Always work with column 0 so IsUnusedRole / ObjectIdRole are accessible
	const QModelIndex source_index = filter_model->mapToSource(proxy_index).siblingAtColumn(0);
	QStandardItem* item = model->itemFromIndex(source_index);
	if (!item) {
		return;
	}

	QMenu menu;

	const bool is_root = !source_index.parent().isValid();
	if (is_root) {
		if (item->data(IsUnusedRole).toBool()) {
			QAction* delete_action = menu.addAction(QApplication::style()->standardIcon(QStyle::SP_TrashIcon), "Delete file");
			connect(delete_action, &QAction::triggered, [this, item]() {
				const QString path_str = item->text();
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
				model->removeRow(item->row());
				update_status();
			});
		}
	} else {
		const int category = item->data(CategoryRole).toInt();
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
