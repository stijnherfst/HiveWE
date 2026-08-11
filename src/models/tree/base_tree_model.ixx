module;

#include <QAbstractProxyModel>
#include <QPainter>
#include <QFileIconProvider>
#include <QSortFilterProxyModel>
#include <QMimeData>
#include <QMessageBox>

export module BaseTreeModel;

import std;
import SLK;
import Globals;
import TableModel;

export class BaseTreeItem {
  public:
	QVector<BaseTreeItem*> children;
	BaseTreeItem* parent = nullptr;

	explicit BaseTreeItem(BaseTreeItem* parent = nullptr)
		: parent(parent) {
		if (parent != nullptr) {
			parent->appendChild(this);
		}
	}

	~BaseTreeItem() {
		qDeleteAll(children);
	}

	void appendChild(BaseTreeItem* item) {
		item->parent = this;
		children.append(item);
	}

	void removeChild(BaseTreeItem* item) {
		item->parent = nullptr;
		children.removeOne(item);
	}

	int row() const {
		if (parent) {
			return parent->children.indexOf(const_cast<BaseTreeItem*>(this));
		}

		return 0;
	}

	std::string id;
	bool baseCategory = false;
	bool subCategory = false;
};

// The fields that have to change to move an object into a folder, or a reject verdict when that is impossible
export struct DropChange {
	enum class Verdict {
		reject,
		accept,
		confirm
	};

	Verdict verdict = Verdict::reject;
	QString question;										// Only used when the verdict is confirm
	std::vector<std::pair<std::string, std::string>> fields;	// Lowercase column header -> new value
};

export class BaseTreeModel : public QAbstractProxyModel {
	int rowCount(const QModelIndex& parent) const override {
		if (parent.column() > 0) {
			return 0;
		}

		BaseTreeItem* parentItem;

		if (!parent.isValid()) {
			parentItem = rootItem;
		} else {
			parentItem = static_cast<BaseTreeItem*>(parent.internalPointer());
		}

		return parentItem->children.count();
	}

	int columnCount(const QModelIndex& parent) const override {
		return 1;
	}

	Qt::ItemFlags flags(const QModelIndex& index) const override {
		if (!index.isValid()) {
			return Qt::NoItemFlags;
		}

		const BaseTreeItem* item = static_cast<BaseTreeItem*>(index.internalPointer());
		if (item->baseCategory || item->subCategory) {
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
		}

		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
	}

	QModelIndex index(const int row, const int column, const QModelIndex& parent) const override {
		if (!hasIndex(row, column, parent))
			return QModelIndex();

		BaseTreeItem* parentItem;

		if (!parent.isValid()) {
			parentItem = rootItem;
		} else {
			parentItem = static_cast<BaseTreeItem*>(parent.internalPointer());
		}

		BaseTreeItem* childItem = parentItem->children.at(row);
		if (childItem)
			return createIndex(row, column, childItem);
		return QModelIndex();
	}

	QModelIndex parent(const QModelIndex& index) const override {
		if (!index.isValid()) {
			return {};
		}

		BaseTreeItem* childItem = static_cast<BaseTreeItem*>(index.internalPointer());
		BaseTreeItem* parentItem = childItem->parent;

		if (parentItem == rootItem)
			return QModelIndex();

		return createIndex(parentItem->row(), 0, parentItem);
	}

	void rowsInserted(const QModelIndex& parent, int first, int last) {
		assert(first == last);

		const std::string id = slk->index_to_row.at(first);
		BaseTreeItem* parent_item = getFolderParent(id);

		beginInsertRows(createIndex(parent_item->row(), 0, parent_item), parent_item->children.size(), parent_item->children.size());
		BaseTreeItem* item = new BaseTreeItem(parent_item);
		item->id = id;
		items.emplace(id, item);
		endInsertRows();
	}

	void rowsRemoved(const QModelIndex& parent, int first, int last) {
		assert(first == last);

		const std::string id = slk->index_to_row.at(first);
		BaseTreeItem* child = items.at(id);

		BaseTreeItem* parent_item = child->parent;
		const int row = parent_item->children.indexOf(child);

		beginRemoveRows(createIndex(parent_item->row(), 0, parent_item), row, row);
		parent_item->removeChild(child);
		items.erase(id);
		delete child;
		endRemoveRows();
	}

	void sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
		Q_ASSERT(topLeft.isValid());
		Q_ASSERT(topLeft.model() == this->sourceModel());
		Q_ASSERT(bottomRight.isValid());
		Q_ASSERT(bottomRight.model() == this->sourceModel());

		// If the changed field is one of those that determine the item's location in the tree we have to move it
		if (std::find(categoryChangeFields.begin(), categoryChangeFields.end(), slk->index_to_column[topLeft.column()]) != categoryChangeFields.end()) {
			const std::string id = slk->index_to_row.at(topLeft.row());

			BaseTreeItem* parent_item = items.at(id)->parent;
			const int row = parent_item->children.indexOf(items.at(id));

			BaseTreeItem* new_parent = getFolderParent(id);

			// beginMoveRows refuses a move where the source and destination parent are the same
			if (new_parent != parent_item) {
				const QModelIndex source_parent = createIndex(parent_item->row(), 0, parent_item);
				const QModelIndex target_parent = createIndex(new_parent->row(), 0, new_parent);

				beginMoveRows(source_parent, row, row, target_parent, new_parent->children.size());
				BaseTreeItem* child = parent_item->children[row];
				parent_item->removeChild(child);
				new_parent->appendChild(child);
				endMoveRows();
			}
		}

		emit dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), roles);
	}

	// BaseTreeItem* newItem(std::string id);
	// void removeItem(std::string id);

	virtual BaseTreeItem* getFolderParent(const std::string& id) const {
		return nullptr;
	};

	// The reverse of getFolderParent. Returns which fields have to change to move the object into the folder,
	// or a reject verdict when the folder can not be expressed as a set of field values for this object
	virtual DropChange prepareDrop(const std::string& id, const BaseTreeItem* target) const {
		return {};
	};

	std::vector<std::string> decodeIds(const QMimeData* data) const {
		std::vector<std::string> ids;
		for (const QByteArray& id : data->data(mimeType).split('\n')) {
			if (!id.isEmpty()) {
				ids.emplace_back(id.toStdString());
			}
		}
		return ids;
	}

	bool fieldEquals(const std::string& column, const std::string& id, const std::string& value) const {
		// Boolean fields are often left empty instead of being set to 0, so comparing the strings would cause
		// a pointless write that marks the object as modified
		if (value == "0" || value == "1") {
			return slk->data<bool>(column, id) == (value == "1");
		}

		return slk->data<std::string_view>(column, id) == value;
	}

	using DropChanges = std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>;

	// Writing through the source model instead of straight to the shadow data makes sure that the tree reparents
	// the object and that the world picks up the change
	void applyFields(const std::string& id, const std::vector<std::pair<std::string, std::string>>& fields) {
		for (const auto& [column, value] : fields) {
			if (!slk->column_headers.contains(column) || fieldEquals(column, id, value)) {
				continue;
			}

			const QModelIndex index = sourceModel()->index(slk->row_headers.at(id), slk->column_headers.at(column));
			sourceModel()->setData(index, QString::fromStdString(value), Qt::EditRole);
		}
	}

	void applyDrop(const DropChanges& changes) {
		for (const auto& [id, fields] : changes) {
			if (!slk->row_headers.contains(id)) {
				continue;
			}

			applyFields(id, fields);
		}
	}

	// Instead of moving the dragged objects we create a custom copy of each of them in the target folder
	void applyCopy(const DropChanges& changes) {
		TableModel* table = static_cast<TableModel*>(sourceModel());

		for (const auto& [id, fields] : changes) {
			if (!slk->row_headers.contains(id)) {
				continue;
			}

			// Whether a unit is a hero follows from the capitalization of its ID, so the copy keeps it
			const std::string new_id = get_unique_id(!islower(id.front()));
			table->copyRow(id, new_id);
			applyFields(new_id, fields);
		}
	}

	// Gathers the changes for every dragged object that can be moved into the target folder
	DropChanges gatherDrop(const QMimeData* data, const QModelIndex& parent, bool copy, QString& question) const {
		DropChanges changes;

		if (!data->hasFormat(mimeType) || !parent.isValid()) {
			return changes;
		}

		const BaseTreeItem* target = static_cast<BaseTreeItem*>(parent.internalPointer());
		if (!target->baseCategory && !target->subCategory) {
			return changes;
		}

		for (const std::string& id : decodeIds(data)) {
			if (!items.contains(id)) {
				continue;
			}

			DropChange change = prepareDrop(id, target);
			if (change.verdict == DropChange::Verdict::reject) {
				continue;
			}

			// A copy leaves every existing object alone, so there is nothing to warn about
			if (change.verdict == DropChange::Verdict::confirm && !copy && question.isEmpty()) {
				question = change.question;
			}

			changes.emplace_back(id, std::move(change.fields));
		}

		return changes;
	}

  public:
	explicit BaseTreeModel(QObject* parent = nullptr)
		: QAbstractProxyModel(parent) {
		rootItem = new BaseTreeItem();

		QFileIconProvider icons;
		folderIcon = icons.icon(QFileIconProvider::Folder);
	}

	~BaseTreeModel() {
		delete rootItem;
	}

	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid()) {
			return {};
		}

		BaseTreeItem* item = static_cast<BaseTreeItem*>(index.internalPointer());

		switch (role) {
			case Qt::DecorationRole:
				if (item->baseCategory || item->subCategory) {
					return folderIcon;
				}
				if (slk->column_headers.contains("art")) {
					return sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("art")), role);
				} else {
					return sourceModel()->data(sourceModel()->index(slk->row_headers.at(item->id), slk->column_headers.at("buffart")), role);
				}
			case Qt::ForegroundRole:
				if (item->baseCategory || item->subCategory) {
					return {};
				}

				if (slk->shadow_data.contains(item->id)) {
					return QColor("violet");
				} else {
					return {};
				}
			case Qt::ToolTipRole:
				if (item->baseCategory || item->subCategory) {
					return {};
				}

				return "ID: " + QString::fromStdString(item->id);
			default:
				return {};
		}
	}

	QStringList mimeTypes() const override {
		return { mimeType };
	}

	QMimeData* mimeData(const QModelIndexList& indexes) const override {
		QByteArray encoded;

		for (const QModelIndex& index : indexes) {
			if (!index.isValid()) {
				continue;
			}

			const BaseTreeItem* item = static_cast<BaseTreeItem*>(index.internalPointer());
			if (item->baseCategory || item->subCategory) {
				continue;
			}

			encoded += QByteArray::fromStdString(item->id) + '\n';
		}

		if (encoded.isEmpty()) {
			return nullptr;
		}

		QMimeData* result = new QMimeData;
		result->setData(mimeType, encoded);
		return result;
	}

	Qt::DropActions supportedDropActions() const override {
		return Qt::MoveAction | Qt::CopyAction;
	}

	Qt::DropActions supportedDragActions() const override {
		return Qt::MoveAction | Qt::CopyAction;
	}

	bool canDropMimeData(const QMimeData* data, const Qt::DropAction action, const int row, const int column, const QModelIndex& parent) const override {
		QString question;
		return !gatherDrop(data, parent, action == Qt::CopyAction, question).empty();
	}

	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override {
		const bool copy = action == Qt::CopyAction;

		QString question;
		DropChanges changes = gatherDrop(data, parent, copy, question);

		if (changes.empty()) {
			return false;
		}

		if (copy) {
			applyCopy(changes);
		} else if (question.isEmpty()) {
			applyDrop(changes);
		} else {
			// Opening a modal dialog from within a drop event is unreliable on Windows because the drag is still
			// running, so ask once the event has been handled
			QMetaObject::invokeMethod(
				this,
				[this, changes = std::move(changes), question] {
					const int answer = QMessageBox::question(nullptr, "Change category", question, QMessageBox::Yes | QMessageBox::No);
					if (answer == QMessageBox::Yes) {
						applyDrop(changes);
					}
				},
				Qt::QueuedConnection
			);
		}

		// We moved or copied the rows ourselves by changing the fields, returning true would make the view remove
		// the dragged rows afterwards
		return false;
	}

	QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override {
		if (!sourceIndex.isValid()) {
			return {};
		}

		const std::string id = slk->index_to_row.at(sourceIndex.row());
		const BaseTreeItem* parent_item = items.at(id)->parent;
		const int row = parent_item->children.indexOf(items.at(id));
		return createIndex(row, 0, items.at(id));
	}

	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override {
		if (!proxyIndex.isValid()) {
			return {};
		}

		BaseTreeItem* item = static_cast<BaseTreeItem*>(proxyIndex.internalPointer());

		if (item->baseCategory || item->subCategory) {
			return {};
		}

		if (slk->column_headers.contains("name")) {
			return createIndex(slk->row_headers.at(item->id), slk->column_headers.at("name"), item);
		} else {
			return createIndex(slk->row_headers.at(item->id), slk->column_headers.at("bufftip"), item);
		}
	}

	void setSourceModel(QAbstractItemModel* sourceModel) override {
		beginResetModel();

		if (this->sourceModel()) {
			disconnect(sourceModel, &QAbstractItemModel::rowsInserted, this, &BaseTreeModel::rowsInserted);
			disconnect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &BaseTreeModel::rowsRemoved);
			disconnect(sourceModel, &QAbstractItemModel::dataChanged, this, &BaseTreeModel::sourceDataChanged);
		}

		QAbstractProxyModel::setSourceModel(sourceModel);

		connect(sourceModel, &QAbstractItemModel::rowsInserted, this, &BaseTreeModel::rowsInserted);
		connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &BaseTreeModel::rowsRemoved);
		connect(sourceModel, &QAbstractItemModel::dataChanged, this, &BaseTreeModel::sourceDataChanged);

		endResetModel();
	}

	QModelIndex getIdIndex(const std::string& id) {
		const BaseTreeItem* parent_item = items.at(id)->parent;
		const int row = parent_item->children.indexOf(items.at(id));
		return createIndex(row, 0, items.at(id));
	}

	BaseTreeItem* rootItem;
	QIcon folderIcon;

	std::vector<std::string> categoryChangeFields;

  protected:
	slk::SLK* slk;
	std::unordered_map<std::string, BaseTreeItem*> items;

	// Dragged objects are encoded as their ids separated by newlines. Every tree has its own mime type so that
	// objects can not be dragged from one type of tree into another
	QString mimeType = "application/x-hivewe-objects";
};

export class BaseFilter : public QSortFilterProxyModel {
	Q_OBJECT

	bool filterCustom = false;

  public:
	slk::SLK* slk;

	using QSortFilterProxyModel::QSortFilterProxyModel;

	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
		QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
		BaseTreeItem* item = static_cast<BaseTreeItem*>(index0.internalPointer());

		if (filterCustom) {
			if (item->baseCategory || item->subCategory) {
				return false;
			}

			if (!(slk->shadow_data.contains(item->id) && slk->shadow_data.at(item->id).contains("oldid"))) {
				return false;
			}
		}

		return sourceModel()->data(index0).toString().contains(filterRegularExpression());
	}

  public slots:
	void setFilterCustom(const bool filter) {
		beginFilterChange();
		filterCustom = filter;
		endFilterChange(Direction::Rows);
	}
};

#include "base_tree_model.moc"