#include "TableModel.h"
#include "QIconResource.h"
#include "HiveWE.h"

std::unordered_map<std::string, std::shared_ptr<QIconResource>> path_to_icon;

TableModel::TableModel(slk::SLK2* slk, slk::SLK2* meta_slk, QObject* parent) : QAbstractTableModel(parent), slk(slk), meta_slk(meta_slk) {
	for (const auto& [key, index] : meta_slk->row_headers) {
		meta_field_to_key.emplace(to_lowercase_copy(meta_slk->data("field", key)), key);
	}

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

	switch (role) {
		case Qt::DisplayRole: {
			const std::string field = slk->index_to_column.at(index.column());
			if (!meta_field_to_key.contains(field)) {
				return QString::fromStdString(slk->data(index.column(), index.row()));
			}

			const std::string type = meta_slk->data("type", meta_field_to_key.at(field));
			if (type == "bool") {
				return QString::fromStdString(slk->data(index.column(), index.row())) == "1" ? "true" : "false";
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
			}

			return QString::fromStdString(slk->data(index.column(), index.row()));
		}
		case Qt::EditRole:
			return QString::fromStdString(slk->data(index.column(), index.row()));
		case Qt::CheckStateRole: {
			const std::string field = slk->index_to_column.at(index.column());
			if (!meta_field_to_key.contains(field)) {
				return {};
			}

			const std::string type = meta_slk->data("type", meta_field_to_key.at(field));
			if (type != "bool") {
				return {};
			}

			return (slk->data(index.column(), index.row()) == "1") ? Qt::Checked : Qt::Unchecked;
		}
		case Qt::DecorationRole:
			const std::string field = slk->index_to_column.at(index.column());
			if (!meta_field_to_key.contains(field)) {
				return {};
			}

			const std::string type = meta_slk->data("type", meta_field_to_key.at(field));
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
		case Qt::CheckStateRole:
		{
			const std::string field = slk->data(index.column(), 0);
			if (!meta_field_to_key.contains(field)) {
				return {};
			}

			const std::string type = meta_slk->data("type", meta_field_to_key.at(field));
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

	int flags = QAbstractTableModel::flags(index);

	const std::string field = slk->data(index.column(), 0);
	if (meta_field_to_key.contains(field)) {
		const std::string type = meta_slk->data("type", meta_field_to_key.at(field));
		if (type == "bool") {
			flags |= Qt::ItemIsUserCheckable;
		}
	}

	if (!(flags & Qt::ItemIsUserCheckable)) {
		flags |= Qt::ItemIsEditable;
	}

	return flags;
}