#pragma once

#include <QSortFilterProxyModel>

#ifdef __linux__
#include "std_compat.h"
#endif
#include <QIcon>
#ifndef __linux__
import std;
#endif
import QIconResource;
import ResourceManager;
import Globals;
#include "models/list/base_list_model.h"
class DoodadListModel: public BaseListModel {

  public:
	explicit DoodadListModel(QObject* parent) : BaseListModel(doodads_slk, parent) {
		for (auto&& [key, value] : world_edit_data.section("DoodadCategories")) {
			const std::string tileset_key = value.front();
			icons[key.front()] = resource_manager.load<QIconResource>(value[1]).value();
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
			case Qt::UserRole:
				return QString::fromStdString("doodads/" + doodads_slk.data("category", index.row()) + "/" + doodads_slk.index_to_row.at(index.row()));
			case Qt::DecorationRole: {
				const char category = doodads_slk.data<std::string_view>("category", index.row()).front();
				if (icons.contains(category)) {
					return icons.at(category)->icon();
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

class DoodadListFilter : public QSortFilterProxyModel {
	[[nodiscard]] bool filterAcceptsRow(const int sourceRow, [[maybe_unused]] const QModelIndex& sourceParent) const override {
		if (!filterRegularExpression().pattern().isEmpty()) {
			if (QString::fromStdString(doodads_slk.index_to_row.at(sourceRow)).contains(filterRegularExpression())) {
				return true;
			}

			const QModelIndex source_index = sourceModel()->index(sourceRow, 0);
			return source_index.data().toString().contains(filterRegularExpression());
		}

		if (filterCategory) {
			if (doodads_slk.data<std::string_view>("category", sourceRow) != filterCategory->toStdString()) {
				return false;
			}
		}

		if (filterTileset) {
			const std::string_view tilesets = doodads_slk.data<std::string_view>("tilesets", sourceRow);
			if (tilesets.find('*') == std::string::npos && tilesets.find(*filterTileset) == std::string::npos && filterTileset != '*') {
				return false;
			}
		}

		return true;
	}

	bool lessThan(const QModelIndex& left, const QModelIndex& right) const override {
		return doodads_slk.data<std::string_view>("name", left.row()) < doodads_slk.data<std::string_view>("name", right.row());
	}

	std::optional<QString> filterCategory;
	std::optional<char> filterTileset;

public:
	void setFilterCategory(const QString& category) {
		beginFilterChange();
		filterCategory = category;
		endFilterChange(Direction::Rows);
	}

	void setFilterTileset(const char tileset) {
		beginFilterChange();
		filterTileset = tileset;
		endFilterChange(Direction::Rows);
	}

	using QSortFilterProxyModel::QSortFilterProxyModel;
};
