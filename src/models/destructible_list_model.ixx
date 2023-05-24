module;

#include <QSize>
#include <QMargins>
#include <QIcon>
#include <QObject>
#include <QModelIndex>
#include <QIdentityProxyModel>
#include <QSortFilterProxyModel>

#include "globals.h"

export module DestructableListModel;

import ResourceManager;
import QIconResource;

export class DestructableListModel : public QIdentityProxyModel {
	Q_OBJECT

  public:
	explicit DestructableListModel(QObject* parent = nullptr)
		: QIdentityProxyModel(parent) {
		for (auto&& [key, value] : world_edit_data.section("DestructibleCategories")) {
			const std::string tileset_key = value.front();
			icons[key.front()] = resource_manager.load<QIconResource>(value[1]);
		}
	}

	QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override {
		if (!sourceIndex.isValid()) {
			return {};
		}

		return createIndex(sourceIndex.row(), 0);
	}

	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override {
		if (!proxyIndex.isValid()) {
			return {};
		}

		return sourceModel()->index(proxyIndex.row(), destructibles_slk.column_headers.at("name"));
	}

	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid()) {
			return {};
		}

		switch (role) {
			case Qt::DisplayRole:
				return sourceModel()->data(mapToSource(index), role).toString() + " " + QString::fromStdString(destructibles_slk.data("editorsuffix", index.row()));
			case Qt::DecorationRole: {
				char category = destructibles_slk.data("category", index.row()).front();
				if (icons.contains(category)) {
					return icons.at(category)->icon;
				} else {
					return {};
				}
			}
			case Qt::SizeHintRole:
				return QSize(0, 16);
			default:
				return sourceModel()->data(mapToSource(index), role);
		}
	}

	Qt::ItemFlags flags(const QModelIndex& index) const override {
		if (!index.isValid()) {
			return Qt::NoItemFlags;
		}

		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	int rowCount(const QModelIndex& parent) const override {
		return sourceModel()->rowCount();
	}

	int columnCount(const QModelIndex& parent) const override {
		return 1;
	}

	QModelIndex DestructableListModel::index(int row, int column, const QModelIndex& parent = QModelIndex()) const override {
		return createIndex(row, column);
	}

	QModelIndex DestructableListModel::parent(const QModelIndex& child) const override {
		return QModelIndex();
	}

  private:
	std::unordered_map<char, std::shared_ptr<QIconResource>> icons;
};

class DestructableListFilter : public QSortFilterProxyModel {
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
		QModelIndex index0 = sourceModel()->index(sourceRow, 0);

		if (!filterRegularExpression().pattern().isEmpty()) {
			return sourceModel()->data(index0).toString().contains(filterRegularExpression());
		} else {
			const std::string tilesets = destructibles_slk.data("tilesets", sourceRow);
			return QString::fromStdString(destructibles_slk.data("category", sourceRow)) == filterCategory && (tilesets.find('*') != std::string::npos || tilesets.find(filterTileset) != std::string::npos || filterTileset == '*');
			;
		}
	}

	bool DestructableListFilter::lessThan(const QModelIndex& left, const QModelIndex& right) const override {
		return destructibles_slk.data("name", left.row()) < destructibles_slk.data("name", right.row());
	} 

	QString filterCategory = "";
	char filterTileset = '*';

  public:
	void setFilterCategory(QString category) {
		filterCategory = category;
		invalidateFilter();
	}

	void setFilterTileset(char tileset) {
		filterTileset = tileset;
		invalidateFilter();
	}

	using QSortFilterProxyModel::QSortFilterProxyModel;
};

#include "destructible_list_model.moc"