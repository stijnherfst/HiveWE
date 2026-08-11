#include "asset_manager.h"

#include <QApplication>
#include <QSizePolicy>
#include <QHBoxLayout>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSplitter>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

#include "model_editor/model_editor.h"
#include "object_editor/model_grid_glwidget.h"

import std;
import SLK;
import Map;
import MapGlobal;
import Globals;
import TableModel;
import ResourceManager;
import WindowHandler;
import Texture;
import AspectRatioPixmapLabel;
import Timer;
import "object_editor/object_editor.h";

namespace fs = std::filesystem;

// Per-severity colours for the validation shorthand, matching mdx::ValidationSeverity order
static constexpr QColor error_color(200, 40, 40);
static constexpr QColor severe_color(200, 120, 0);
static constexpr QColor warning_color(180, 150, 0);
static constexpr QColor unused_severity_color(130, 130, 130);
static constexpr QColor clean_color(40, 160, 40);

static QColor severity_token_color(QChar letter) {
	switch (letter.unicode()) {
		case 'E': return error_color;
		case 'S': return severe_color;
		case 'W': return warning_color;
		case 'U': return unused_severity_color;
	}
	return {};
}

// Draws the validation shorthand ("2E 1S 3W", "✓", "err") with each token in its severity colour.
class ValidationDelegate : public QStyledItemDelegate {
  public:
	using QStyledItemDelegate::QStyledItemDelegate;

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
		QStyleOptionViewItem opt = option;
		initStyleOption(&opt, index);
		const QString text = opt.text;
		opt.text.clear();
		// Let the style paint the background/selection, then draw the coloured tokens ourselves
		QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
		style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

		if (text.isEmpty()) {
			return;
		}

		QColor color;
		if (text == "✓") {
			color = clean_color;
		} else if (text == "err") {
			color = error_color;
		}

		const QRect rect = opt.rect.adjusted(4, 0, -4, 0);
		painter->save();
		painter->setFont(opt.font);
		int x = rect.left();
		if (!color.isValid()) {
			// Space-separated tokens like "2E", each coloured by its trailing severity letter
			const QStringList tokens = text.split(' ', Qt::SkipEmptyParts);
			const int space = painter->fontMetrics().horizontalAdvance(' ');
			for (const QString& token : tokens) {
				painter->setPen(severity_token_color(token.back()));
				painter->drawText(QRect(x, rect.top(), rect.width(), rect.height()), Qt::AlignVCenter | Qt::AlignLeft, token);
				x += painter->fontMetrics().horizontalAdvance(token) + space;
			}
		} else {
			painter->setPen(color);
			painter->drawText(rect, Qt::AlignVCenter | Qt::AlignLeft, text);
		}
		painter->restore();
	}
};

bool AssetFilterModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
	// Sort the Size column by raw byte count
	if (left.column() == 1) {
		return left.data(SizeRole).toULongLong() < right.data(SizeRole).toULongLong();
	}
	// Sort the Usages column numerically
	if (left.column() == 2) {
		return left.data(Qt::DisplayRole).toInt() < right.data(Qt::DisplayRole).toInt();
	}
	// Sort the Validation column by weighted severity score
	if (left.column() == 3) {
		return left.data(ValidationSortRole).toInt() < right.data(ValidationSortRole).toInt();
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
		// For child rows check the top level file item, since recursive filtering would
		// otherwise resurface a hidden file whose children match the text filter
		QModelIndex file_index = source_parent.isValid() ? source_parent : sourceModel()->index(source_row, 0, source_parent);
		while (file_index.parent().isValid()) {
			file_index = file_index.parent();
		}
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
	resize(1200, 850);

	auto* layout = new QVBoxLayout(this);

	// Left side: search bar, filters, status and the file tree, stacked vertically
	auto* left_panel = new QWidget(this);
	auto* left_layout = new QVBoxLayout(left_panel);
	left_layout->setContentsMargins(0, 0, 0, 0);

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

	left_layout->addLayout(search_bar);

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

	left_layout->addLayout(action_bar);

	status_label = new QLabel(this);
	left_layout->addWidget(status_label);

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
	tree_view->setItemDelegateForColumn(3, new ValidationDelegate(this));
	tree_view->header()->setStretchLastSection(false);
	tree_view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	tree_view->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	tree_view->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	tree_view->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	left_layout->addWidget(tree_view, 1);

	// Right-side live preview of the selected file
	preview_host = new QWidget(this);
	auto* preview_host_layout = new QVBoxLayout(preview_host);
	preview_host_layout->setContentsMargins(0, 0, 0, 0);

	// Create the OpenGL preview widget up front (with no model) so the window's OpenGL surface is
	// established during the initial show(). Otherwise the first model click promotes the window to
	// an OpenGL-capable surface while it is already visible, flashing the whole window white.
	show_empty_preview();

	open_model_editor_button = new QPushButton("Open in Model Editor", this);
	open_model_editor_button->setIcon(QIcon("data/icons/ribbon/model_editor.png"));
	open_model_editor_button->setToolTip("Open in Model Editor");
	open_model_editor_button->hide();

	auto* preview_panel = new QWidget(this);
	auto* preview_layout = new QVBoxLayout(preview_panel);
	preview_layout->setContentsMargins(0, 0, 0, 0);
	preview_layout->addWidget(preview_host, 1);
	preview_layout->addWidget(open_model_editor_button);

	auto* splitter = new QSplitter(Qt::Horizontal, this);
	splitter->addWidget(left_panel);
	splitter->addWidget(preview_panel);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 1);
	// Start with the preview roughly square: its width matches the available window height.
	splitter->setSizes({520, 680});
	layout->addWidget(splitter);

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
	connect(tree_view->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current, const QModelIndex&) {
		show_preview(current);
	});
	connect(open_model_editor_button, &QPushButton::clicked, this, &AssetManager::open_selected_in_model_editor);

	refresh();
	show();
}

void AssetManager::refresh() const {
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
	if (source_index.data(FileRowRole).toInt() >= 0) {
		return; // file item — nothing to open in the Object Editor
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

void AssetManager::clear_preview() {
	delete preview_widget;
	preview_widget = nullptr;
	current_model_path.clear();
	open_model_editor_button->hide();
}

void AssetManager::show_empty_preview() {
	// An empty single-preview GL widget: keeps the window's OpenGL surface alive (no white flash on
	// the next model click) while telling the user why the preview area is blank.
	auto* placeholder = new ModelGridGLWidget({}, preview_host, true);
	placeholder->set_empty_message("Select an asset to show a preview");
	preview_widget = placeholder;
	preview_host->layout()->addWidget(placeholder);
}

void AssetManager::show_preview(const QModelIndex& current) {
	clear_preview();

	if (!current.isValid()) {
		show_empty_preview();
		return;
	}
	const QModelIndex source_index = filter_model->mapToSource(current).siblingAtColumn(0);
	const int file_row = source_index.data(FileRowRole).toInt();
	if (file_row < 0) {
		show_empty_preview(); // an object or a file that is not in the map, not previewable
		return;
	}

	const AssetTreeModel::FileNode& node = model->file(file_row);

	std::string ext = fs::path(node.path).extension().string();
	for (auto& c : ext) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}

	const auto set_preview = [this](QWidget* widget) {
		preview_widget = widget;
		preview_host->layout()->addWidget(widget);
	};

	if (ext == ".mdx" || ext == ".mdl") {
		current_model_path = QString::fromStdString(node.path);
		open_model_editor_button->show();
		set_preview(new ModelGridGLWidget({ModelEntry{node.path, ModelCategory::Map}}, preview_host, true));
		return;
	}

	if (ext == ".blp" || ext == ".tga" || ext == ".dds") {
		auto* label = new AspectRatioPixmapLabel(preview_host);
		label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		if (const auto tex = resource_manager.load<Texture>(node.path)) {
			const QImage image(tex.value()->data.data(), tex.value()->width, tex.value()->height,
							   tex.value()->channels == 3 ? QImage::Format_RGB888 : QImage::Format_RGBA8888);
			label->setPixmap(QPixmap::fromImage(image));
		}
		set_preview(label);
		return;
	}

	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
		// Not handled by Texture/hierarchy decode; Qt loads these directly from disk
		auto* label = new AspectRatioPixmapLabel(preview_host);
		label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		const QString full_path = QString::fromStdString((map->filesystem_path / node.path).string());
		label->setPixmap(QPixmap(full_path));
		set_preview(label);
		return;
	}

	auto* label = new QLabel("No preview available", preview_host);
	label->setAlignment(Qt::AlignCenter);
	set_preview(label);
}

void AssetManager::open_selected_in_model_editor() {
	if (current_model_path.isEmpty()) {
		return;
	}
	bool created = false;
	auto* model_editor = window_handler.create_or_raise<ModelEditor>(nullptr, created);
	const auto opened = model_editor->open_model_docked(map->filesystem_path / current_model_path.toStdString(), true);
	if (!opened) {
		QMessageBox::critical(this, "Error opening model",
							  QString::fromStdString(std::format("Failed to open model with: {}", opened.error())));
	}
}

void AssetManager::show_context_menu(const QPoint& pos) {
	const QModelIndex proxy_index = tree_view->indexAt(pos);
	if (!proxy_index.isValid()) {
		return;
	}

	// Always work with column 0 so IsUnusedRole / ObjectIdRole are accessible
	const QModelIndex source_index = filter_model->mapToSource(proxy_index).siblingAtColumn(0);

	QMenu menu;

	const int file_row = source_index.data(FileRowRole).toInt();
	if (file_row >= 0) {
		// Deleting is only offered on the top level item, the same file can be shown at many places
		if (!source_index.parent().isValid() && source_index.data(IsUnusedRole).toBool()) {
			QAction* delete_action = menu.addAction(QApplication::style()->standardIcon(QStyle::SP_TrashIcon), "Delete file");
			connect(delete_action, &QAction::triggered, [this, row = file_row]() {
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
