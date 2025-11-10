module;

#include <QSortFilterProxyModel>

export module DoodadListModel;

import std;
import QIconResource;
import ResourceManager;
import Globals;
import BaseListModel;

export class DoodadListModel: public BaseListModel {
	Q_OBJECT

  public:
	explicit DoodadListModel(QObject* parent) : BaseListModel(doodads_slk, parent) {
		for (auto&& [key, value] : world_edit_data.section("DoodadCategories")) {
			const std::string tileset_key = value.front();
			icons[key.front()] = resource_manager.load<QIconResource>(value[1]);
		}
	}

	QModelIndex mapToSource(const QModelIndex& proxyIndex) const override {
		if (!proxyIndex.isValid()) {
			return {};
		}

		return sourceModel()->index(proxyIndex.row(), doodads_slk.column_headers.at("name"));
	}

	QVariant data(const QModelIndex& index, int role) const override {
		if (!index.isValid()) {
			return {};
		}

		switch (role) {
			case Qt::DisplayRole:
				return sourceModel()->data(mapToSource(index), role).toString();
			case Qt::DecorationRole: {
				char category = doodads_slk.data("category", index.row()).front();
				if (icons.contains(category)) {
					return icons.at(category)->icon;
				} else {
					return {};
				}
			}
			default:
				return BaseListModel::data(index, role);
		}
	}

  private:
	std::unordered_map<char, std::shared_ptr<QIconResource>> icons;
};

export class DoodadListFilter : public QSortFilterProxyModel {
	bool filterAcceptsRow(const int sourceRow, const QModelIndex& sourceParent) const override {
		if (QString::fromStdString(doodads_slk.index_to_row.at(sourceRow)).contains(filterRegularExpression())) {
			return true;
		}

		if (!filterRegularExpression().pattern().isEmpty()) {
			const QModelIndex source_index = sourceModel()->index(sourceRow, 0);
			return source_index.data().toString().contains(filterRegularExpression());
		} else {
			const std::string tilesets = doodads_slk.data("tilesets", sourceRow);
			return QString::fromStdString(doodads_slk.data("category", sourceRow)) == filterCategory && (tilesets.find('*') != std::string::npos || tilesets.find(filterTileset) != std::string::npos || filterTileset == '*');
		}
	}

	bool lessThan(const QModelIndex& left, const QModelIndex& right) const override {
		return doodads_slk.data("name", left.row()) < doodads_slk.data("name", right.row());
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

#include "doodad_list_model.moc"
