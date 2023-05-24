#include "table_model.h"
#include "globals.h"

#include <absl/strings/str_split.h>

#include <map_global.h>

import QIconResource;

std::unordered_map<std::string, std::shared_ptr<QIconResource>> path_to_icon;

TableModel::TableModel(slk::SLK* slk, slk::SLK* meta_slk, QObject* parent) : QAbstractTableModel(parent), slk(slk), meta_slk(meta_slk) {
	/*for (const auto& [key, index] : meta_slk->row_headers) {
		meta_field_to_key.emplace(to_lowercase_copy(meta_slk->data("field", key)), key);
	}*/

	invalid_icon = resource_manager.load<QIconResource>("ReplaceableTextures/WorldEditUI/DoodadPlaceholder.dds");
}

int TableModel::rowCount(const QModelIndex& parent) const {
	return slk->rows();
}

int TableModel::columnCount(const QModelIndex& parent) const {
	return slk->columns();
}

QVariant TableModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return {};
	}

	const std::string& id = slk->index_to_row.at(index.row());
	const std::string& field = slk->index_to_column.at(index.column());
	const std::string meta_id = fieldToMetaID(id, field);

	switch (role) {
		case Qt::DisplayRole: {
			const std::string field_data = slk->data(index.column(), index.row());

			if (field_data.starts_with("TRIGSTR")) {
				return QString::fromStdString(map->trigger_strings.string(field_data));
			}

			const std::string type = meta_slk->data("type", meta_id);
			if (type == "bool") {
				return field_data == "1" ? "true" : "false";
			} else if (type == "unitList") {
				std::vector<std::string_view> parts = absl::StrSplit(field_data, ',');
				QString result;
				for (size_t i = 0; i < parts.size(); i++) {
					if (!units_slk.row_headers.contains(parts[i])) {
						continue;
					}
					result += units_table->data(units_table->index(units_slk.row_headers.at(parts[i]), units_slk.column_headers.at("name")), role).toString();
					if (i < parts.size() - 1) {
						result += ", ";
					}
				}
				return result;
			} else if (type == "abilityList" || type == "abilitySkinList" || type == "heroAbilityList") {
				std::vector<std::string_view> parts = absl::StrSplit(field_data, ',');
				QString result;
				for (size_t i = 0; i < parts.size(); i++) {
					if (!abilities_slk.row_headers.contains(parts[i])) {
						continue;
					}
					result += abilities_table->data(abilities_table->index(abilities_slk.row_headers.at(parts[i]), abilities_slk.column_headers.at("name")), role).toString();
					if (i < parts.size() - 1) {
						result += ", ";
					}
				}
				return result;
			} else if (type == "upgradeList" ) {
				std::vector<std::string_view> parts = absl::StrSplit(field_data, ',');
				QString result;
				for (size_t i = 0; i < parts.size(); i++) {
					if (!upgrade_slk.row_headers.contains(parts[i])) {
						continue;
					}
					result += upgrade_table->data(upgrade_table->index(upgrade_slk.row_headers.at(parts[i]), upgrade_slk.column_headers.at("name1")), role).toString();
					if (i < parts.size() - 1) {
						result += ", ";
					}
				}
				return result;
			} else if (type == "buffList") {
				std::vector<std::string_view> parts = absl::StrSplit(field_data, ',');
				QString result;
				for (size_t i = 0; i < parts.size(); i++) {
					if (!buff_slk.row_headers.contains(parts[i])) {
						continue;
					}
					QString editorname = buff_table->data(buff_table->index(buff_slk.row_headers.at(parts[i]), buff_slk.column_headers.at("editorname")), role).toString();
					if (editorname.isEmpty()) {
						result += buff_table->data(buff_table->index(buff_slk.row_headers.at(parts[i]), buff_slk.column_headers.at("bufftip")), role).toString();
					} else {
						result += editorname;
					}

					if (i < parts.size() - 1) {
						result += ", ";
					}
				}
				return result;
			} else if (type == "targetList") {
				std::vector<std::string_view> parts = absl::StrSplit(field_data, ',');
				std::string result;
				for (size_t i = 0; i < parts.size(); i++) {
					for (const auto& [key, value] : unit_editor_data.section(type)) {
						if (key == "NumValues" || key == "Sort" || key.ends_with("_Alt")) {
							continue;
						}
						
						if (value[0] == parts[i]) {
							result += value[1];
							if (i < parts.size() - 1) {
								result += ", ";
							}
						}
					}
				}
				QString result_qstring = QString::fromStdString(result);
				result_qstring.replace('&', "");
				return result_qstring;
			} else if (type == "tilesetList") {
				std::vector<std::string_view> parts = absl::StrSplit(field_data, ',');
				QString result;
				for (size_t i = 0; i < parts.size(); i++) {
					if (parts[i] == "*") {
						result += "All";
					} else {
						result += QString::fromStdString(world_edit_data.data("TileSets", std::string(parts[i])));
					}
					if (i < parts.size() - 1) {
						result += ", ";
					}
				}
				return result;
			} else if (unit_editor_data.section_exists(type)) {
				for (const auto& [key, value] : unit_editor_data.section(type)) {
					if (key == "NumValues" || key == "Sort" || key.ends_with("_Alt")) {
						continue;
					}

					if (slk->data(index.column(), index.row()) == value[0]) {
						QString displayText = QString::fromStdString(value[1]);
						displayText.replace('&', "");
						return displayText;
					}
				}
			} else if (type == "doodadCategory") {
				for (auto&& [key, value] : world_edit_data.section("DoodadCategories")) {
					if (field_data == key) {
						return QString::fromStdString(value[0]);
					}
				}
			} else if (type == "destructableCategory") {
				for (auto&& [key, value] : world_edit_data.section("DestructibleCategories")) {
					if (field_data == key) {
						return QString::fromStdString(value[0]);
					}
				}
			} else if (type == "techList") {
				std::vector<std::string_view> parts = absl::StrSplit(field_data, ',');
				QString result;
				for (size_t i = 0; i < parts.size(); i++) {
					if (units_slk.row_headers.contains(parts[i])) {
						result += units_table->data(units_table->index(units_slk.row_headers.at(parts[i]), units_slk.column_headers.at("name")), role).toString();
					} else if (upgrade_slk.row_headers.contains(parts[i])) {
						result += upgrade_table->data(upgrade_table->index(upgrade_slk.row_headers.at(parts[i]), upgrade_slk.column_headers.at("name1")), role).toString();
					} else {
						result += QString::fromStdString(std::string(parts[i]));
					}
					if (i < parts.size() - 1) {
						result += ", ";
					}
				}

				return result;
			}

			return QString::fromStdString(field_data);
		}
		case Qt::EditRole:
			return QString::fromStdString(slk->data(index.column(), index.row()));
		case Qt::CheckStateRole: {
			const std::string type = meta_slk->data("type", meta_id);
			if (type != "bool") {
				return {};
			}

			return (slk->data(index.column(), index.row()) == "1") ? Qt::Checked : Qt::Unchecked;
		}
		case Qt::DecorationRole:
			const std::string type = meta_slk->data("type", meta_id);
			if (type != "icon") {
				return {};
			}

			fs::path icon = slk->data(index.column(), index.row());
			if (icon.empty()) {
				return invalid_icon->icon;
			}

			if (path_to_icon.contains(icon.string())) {
				return path_to_icon.at(icon.string())->icon;
			}

			if (!hierarchy.file_exists(icon)) {
				icon.replace_extension(".dds");
				if (!hierarchy.file_exists(icon)) {
					return {};
				}
			}

			path_to_icon[icon.string()] = resource_manager.load<QIconResource>(icon);
			return path_to_icon.at(icon.string())->icon;
	}

	return {};
}

bool TableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (!index.isValid()) {
		return {};
	}

	switch (role) {
		case Qt::EditRole:
			slk->set_shadow_data(index.column(), index.row(), value.toString().toStdString());
			emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole, Qt::DecorationRole });
			return true;
		case Qt::CheckStateRole: {
			const std::string& id = slk->index_to_row.at(index.row());
			const std::string& field = slk->index_to_column.at(index.column());
			const std::string type = meta_slk->data("type", fieldToMetaID(id, field));
			if (type != "bool") {
				return false;
			}

			slk->set_shadow_data(index.column(), index.row(), (value.toInt() == Qt::Checked) ? "1" : "0");
			emit dataChanged(index, index, { role });
			return true;
		}

	}
	return false;
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role != Qt::DisplayRole) {
		return {};
	}

	if (orientation == Qt::Orientation::Horizontal) {
		return QString::fromStdString(slk->data(section, 0));
	} else {
		return QString::fromStdString(slk->data(0, section));
	}
}

Qt::ItemFlags TableModel::flags(const QModelIndex& index) const {
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}

	Qt::ItemFlags flags = QAbstractTableModel::flags(index);

	const std::string& id = slk->index_to_row.at(index.row());
	const std::string& field = slk->index_to_column.at(index.column());
	const std::string type = meta_slk->data("type", fieldToMetaID(id, field));
	if (type == "bool") {
		flags |= Qt::ItemIsUserCheckable;
	}

	if (!(flags & Qt::ItemIsUserCheckable)) {
		flags |= Qt::ItemIsEditable;
	}

	return flags;
}

void TableModel::copyRow(std::string_view row_header, std::string_view new_row_header) {
	beginInsertRows(QModelIndex(), rowCount(), rowCount());

	slk->copy_row(row_header, new_row_header, true);
	endInsertRows();
}

void TableModel::deleteRow(const std::string_view row_header) {
	const int row = slk->row_headers.at(row_header);
	beginRemoveRows(QModelIndex(), row , row);

	slk->remove_row(row_header);
	endRemoveRows();
}

std::string TableModel::fieldToMetaID(const std::string& id, const std::string& field) const {
	if (meta_slk->meta_map.contains(field)) {
		return meta_slk->meta_map.at(field);
	}

	const size_t nr_position = field.find_first_of("0123456789");
	const std::string new_field = field.substr(0, nr_position);
	
	if (meta_slk->meta_map.contains(new_field)) {
		return meta_slk->meta_map.at(new_field);
	}

	return meta_slk->meta_map.at(new_field + id);
}

// Returns the model index belonging to the row with the given id
QModelIndex TableModel::rowIDToIndex(const std::string& id) const {
	return createIndex(slk->row_headers.at(id), 0);
}