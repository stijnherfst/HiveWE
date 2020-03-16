#include "UnitModel.h"

std::unordered_map<std::string, std::shared_ptr<QIconResource>> path_to_icon;

int UnitModel::rowCount(const QModelIndex& parent) const {
	return units_slk.rows - 1;
}

int UnitModel::columnCount(const QModelIndex& parent) const {
	return units_slk.columns - 1;
}

QVariant UnitModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return {};
	}

	switch (role) {
		case Qt::DisplayRole:
		{
			const std::string field = units_slk.data(index.column(), 0);
			if (!meta_field_to_index.contains(field)) {
				return QString::fromStdString(units_slk.data(index.column(), index.row()));
			}

			const std::string type = units_meta_slk.data("type", meta_field_to_index.at(field));
			if (type == "bool") {
				return QString::fromStdString(units_slk.data(index.column(), index.row())) == "1" ? "true" : "false";
			}
			else if (type == "int") {
				return QString::fromStdString(units_slk.data(index.column(), index.row())).toInt();
			} else if (type == "unreal") {
				return QString::fromStdString(units_slk.data(index.column(), index.row())).toFloat();
			}

			return QString::fromStdString(units_slk.data(index.column(), index.row()));
		}
		case Qt::EditRole:
			return QString::fromStdString(units_slk.data(index.column(), index.row()));	
		case Qt::CheckStateRole:
		{
			const std::string field = units_slk.data(index.column(), 0);
			if (!meta_field_to_index.contains(field)) {
				return {};
			}

			const std::string type = units_meta_slk.data("type", meta_field_to_index.at(field));
			if (type != "bool") {
				return {};
			}

			return (units_slk.data(index.column(), index.row()) == "1") ? Qt::Checked : Qt::Unchecked;
		}
		case Qt::DecorationRole:
			const std::string field = units_slk.data(index.column(), 0);
			if (!meta_field_to_index.contains(field)) {
				return {};
			}

			const std::string type = units_meta_slk.data("type", meta_field_to_index.at(field));
			if (type != "icon") {
				return {};
			}

			fs::path icon = units_slk.data(index.column(), index.row());
			if (icon.empty()) {
				return {};
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

bool UnitModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (!index.isValid()) {
		return {};
	}

	switch (role) {
		case Qt::EditRole:
			units_slk.set_shadow_data(index.column(), index.row(), value.toString().toStdString());
			emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole, Qt::DecorationRole });
			return true;
		case Qt::CheckStateRole: {
			const std::string field = units_slk.data(index.column(), 0);
			if (!meta_field_to_index.contains(field)) {
				return {};
			}

			const std::string type = units_meta_slk.data("type", meta_field_to_index.at(field));
			if (type != "bool") {
				return false;
			}

			units_slk.set_shadow_data(index.column(), index.row(), (value.toInt() == Qt::Checked) ? "1" : "0");
			emit dataChanged(index, index, { role });
			return true;
		}

	}
	return false;
}

QVariant UnitModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role != Qt::DisplayRole) {
		return {};
	}

	if (orientation == Qt::Orientation::Horizontal) {
		return QString::fromStdString(units_slk.data(section, 0));
	} else {
		return QString::fromStdString(units_slk.data(0, section));
	}
}

Qt::ItemFlags UnitModel::flags(const QModelIndex& index) const {
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}

	int flags = QAbstractTableModel::flags(index);

	const std::string field = units_slk.data(index.column(), 0);
	if (meta_field_to_index.contains(field)) {
		const std::string type = units_meta_slk.data("type", meta_field_to_index.at(field));
		if (type == "bool") {
			flags |= Qt::ItemIsUserCheckable;
		}
	}

	if (!(flags & Qt::ItemIsUserCheckable)) {
		flags |= Qt::ItemIsEditable;
	}

	return flags;
}

UnitModel::UnitModel(QObject* parent) : QAbstractTableModel(parent) {
	for (const auto& [key, index] : units_meta_slk.header_to_row) {
		meta_field_to_index.emplace(to_lowercase_copy(units_meta_slk.data("field", index)), index);
	}
}
