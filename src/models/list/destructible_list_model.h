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
class DestructableListModel: public BaseListModel {

  public:
	explicit DestructableListModel(QObject* parent = nullptr) : BaseListModel(destructibles_slk, parent) {
		for (auto&& [key, value] : world_edit_data.section("DestructibleCategories")) {
			const std::string tileset_key = value.front();
			icons[key.front()] = resource_manager.load<QIconResource>(value[1]).value();
		}
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
				return sourceModel()->data(mapToSource(index), role).toString() + " " + QString::fromUtf8(destructibles_slk.data<std::string_view>("editorsuffix", index.row()));
			case Qt::UserRole:
				return QString::fromStdString("destructibles/" + destructibles_slk.data("category", index.row()) + "/" + destructibles_slk.index_to_row.at(index.row()));
			case Qt::DecorationRole: {
				const char category = destructibles_slk.data<std::string_view>("category", index.row()).front();
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

class DestructableListFilter: public QSortFilterProxyModel {
	[[nodiscard]] bool filterAcceptsRow(const int sourceRow, [[maybe_unused]] const QModelIndex& sourceParent) const override {
		if (!filterRegularExpression().pattern().isEmpty()) {
			if (QString::fromStdString(destructibles_slk.index_to_row.at(sourceRow)).contains(filterRegularExpression())) {
				return true;
			}

			const QModelIndex source_index = sourceModel()->index(sourceRow, 0);
			return source_index.data().toString().contains(filterRegularExpression());
		}

		if (filterCategory) {
			if (destructibles_slk.data<std::string_view>("category", sourceRow) != filterCategory->toStdString()) {
				return false;
			}
		}

		if (filterTileset) {
			const std::string_view tilesets = destructibles_slk.data<std::string_view>("tilesets", sourceRow);
			if (tilesets.find('*') == std::string::npos && tilesets.find(*filterTileset) == std::string::npos && filterTileset != '*') {
				return false;
			}
		}

		return true;
	}

	[[nodiscard]] bool lessThan(const QModelIndex& left, const QModelIndex& right) const override {
		return destructibles_slk.data<std::string_view>("name", left.row()) < destructibles_slk.data<std::string_view>("name", right.row());
	}

	std::optional<QString> filterCategory;
	std::optional<char> filterTileset;

  public:
	void setFilterCategory(const QString& category) {
		beginFilterChange();
		filterCategory = category;
		endFilterChange(Direction::Rows);
	}

	void setFilterTileset(char tileset) {
		beginFilterChange();
		filterTileset = tileset;
		endFilterChange(Direction::Rows);
	}

	using QSortFilterProxyModel::QSortFilterProxyModel;
};
