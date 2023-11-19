module;

#include <QSize>
#include <QMargins>
#include <QIcon>
#include <QAbstractTableModel>
#include "globals.h"
#include <absl/strings/str_split.h>

export module TableModel;

import QIconResource;
import SLK;
import TriggerStrings;
import QIconResource;
import Hierarchy;

std::unordered_map<std::string, std::shared_ptr<QIconResource>> path_to_icon;


class TableModel;

export inline TableModel* units_table;
export inline TableModel* items_table;
export inline TableModel* abilities_table;
export inline TableModel* doodads_table;
export inline TableModel* destructibles_table;
export inline TableModel* upgrade_table;
export inline TableModel* buff_table;

export class TableModel : public QAbstractTableModel {
	std::shared_ptr<QIconResource> invalid_icon;

  public:
	slk::SLK* meta_slk;
	slk::SLK* slk;
	TriggerStrings* trigger_strings;

	explicit TableModel(slk::SLK* slk, slk::SLK* meta_slk, TriggerStrings* trigger_strings, QObject* parent = nullptr)
		: QAbstractTableModel(parent), slk(slk), meta_slk(meta_slk), trigger_strings(trigger_strings) {

		invalid_icon = resource_manager.load<QIconResource>("ReplaceableTextures/WorldEditUI/DoodadPlaceholder.dds");
	}

	int rowCount(const QModelIndex& parent = QModelIndex()) const override {
		return slk->rows();
	}

	int columnCount(const QModelIndex& parent = QModelIndex()) const override {
		return slk->columns();
	}

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
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
					return QString::fromStdString(trigger_strings->string(field_data));
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
						result += units_table->data(parts[i], "name", role).toString();
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
						result += abilities_table->data(parts[i], "name", role).toString();
						if (i < parts.size() - 1) {
							result += ", ";
						}
					}
					return result;
				} else if (type == "upgradeList") {
					std::vector<std::string_view> parts = absl::StrSplit(field_data, ',');
					QString result;
					for (size_t i = 0; i < parts.size(); i++) {
						if (!upgrade_slk.row_headers.contains(parts[i])) {
							continue;
						}
						result += upgrade_table->data(parts[i], "name1", role).toString();
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
						QString editorname = buff_table->data(parts[i], "editorname", role).toString();
						if (editorname.isEmpty()) {
							result += buff_table->data(parts[i], "bufftip", role).toString();
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
							result += units_table->data(parts[i], "name", role).toString();
						} else if (upgrade_slk.row_headers.contains(parts[i])) {
							result += upgrade_table->data(parts[i], "name1", role).toString();
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

	QVariant data(const std::string_view id, const std::string_view field, int role = Qt::DisplayRole) const {
		return data(index(slk->row_headers.at(id), slk->column_headers.at(field)), role);
	}

	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override {
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

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
		if (role != Qt::DisplayRole) {
			return {};
		}

		if (orientation == Qt::Orientation::Horizontal) {
			return QString::fromStdString(slk->data(section, 0));
		} else {
			return QString::fromStdString(slk->data(0, section));
		}
	}

	Qt::ItemFlags flags(const QModelIndex& index) const override {
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

	void copyRow(std::string_view row_header, std::string_view new_row_header) {
		beginInsertRows(QModelIndex(), rowCount(), rowCount());

		slk->copy_row(row_header, new_row_header, true);
		endInsertRows();
	}

	void deleteRow(const std::string_view row_header) {
		const int row = slk->row_headers.at(row_header);
		beginRemoveRows(QModelIndex(), row, row);
		slk->remove_row(row_header);
		endRemoveRows();
	}

	std::string fieldToMetaID(const std::string& id, const std::string& field) const {
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
	QModelIndex rowIDToIndex(const std::string& id) const {
		return createIndex(slk->row_headers.at(id), 0);
	}
};