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

UnitSingleModel::UnitSingleModel(QObject* parent) : QAbstractProxyModel(parent) {
	for (const auto& [key, index] : units_meta_slk.header_to_row) {
		if (units_slk.header_to_column.contains(to_lowercase_copy(units_meta_slk.data("field", key))) && !units_meta_slk.data("id", key).starts_with('i')) {
			id_mapping.push_back(index);
		}
	}

	std::sort(id_mapping.begin(), id_mapping.end(), [&](int left, int right) {
		std::string category = world_edit_data.data("ObjectEditorCategories", units_meta_slk.data("category", left));
		category = string_replaced(category, "&", "");
		std::string left_string = category + " - " + units_meta_slk.data("displayname", left);

		category = world_edit_data.data("ObjectEditorCategories", units_meta_slk.data("category", right));
		category = string_replaced(category, "&", "");
		std::string right_string = category + " - " + units_meta_slk.data("displayname", right);

		return left_string < right_string;
	});
}

QModelIndex UnitSingleModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}
	if (sourceIndex.row() != units_slk.header_to_row.at(unit_id)) {
		return {};
	}

	//std::cout << sourceIndex.row() << "\t" << sourceIndex.column() << "\n";

	int row = -1;
	auto t = units_slk.data(sourceIndex.column(), 0);

	for (const auto& [key, value] : units_meta_slk.header_to_row) {
		if (to_lowercase_copy(units_meta_slk.data("field", value)) == t) {
			row = value;
			break;
		}
	}

	for (int i = 0; i < id_mapping.size(); i++) {
		if (id_mapping[i] == row) {
			//std::cout << "Found " << row << " at " << i << " " << headerData(i, Qt::Vertical, Qt::DisplayRole).toString().toStdString() << " " << units_meta_slk.data("displayname", id_mapping[i]) << "\n";
			return createIndex(i, 0);
		}
	}
	return {};
}

QModelIndex UnitSingleModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	std::string column = to_lowercase_copy(units_meta_slk.data("field", id_mapping[proxyIndex.row()]));
	return sourceModel()->index(units_slk.header_to_row.at(unit_id), units_slk.header_to_column.at(column));
}

QVariant UnitSingleModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role != Qt::DisplayRole) {
		return {};
	}

	if (orientation == Qt::Orientation::Vertical) {
		std::string category = world_edit_data.data("ObjectEditorCategories", units_meta_slk.data("category", id_mapping[section]));
		category = string_replaced(category, "&", "");
		return QString::fromStdString(category + " - " + units_meta_slk.data("displayname", id_mapping[section]) + " (" + units_meta_slk.data("id", id_mapping[section]) + ")");
	} else {
		return "UnitID";
	}
}

int UnitSingleModel::rowCount(const QModelIndex& parent) const {
	return id_mapping.size();
}

int UnitSingleModel::columnCount(const QModelIndex& parent) const {
	return 1;
}

QModelIndex UnitSingleModel::index(int row, int column, const QModelIndex& parent) const {
	return 	createIndex(row, column);
}

QModelIndex UnitSingleModel::parent(const QModelIndex& child) const {
	return QModelIndex();
}

//Qt::ItemFlags UnitSingleModel::flags(const QModelIndex& index) const {
//	if (!index.isValid()) {
//		return Qt::NoItemFlags;
//	}
//
//	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
//}

void UnitSingleModel::setUnitID(const std::string_view unitID) {
	beginResetModel();
	unit_id = unitID;
	endResetModel();
}

void AlterHeader::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const {
	Qt::Alignment align = (Qt::AlignLeft | Qt::AlignVCenter);
	
	if (logicalIndex % 2 > 0) {
		painter->fillRect(rect, palette().color(QPalette::AlternateBase));
	} else {
		painter->fillRect(rect, palette().color(QPalette::Base));
	}

	painter->drawText(rect.adjusted(2 * style()->pixelMetric(QStyle::PM_HeaderMargin, 0, this), 0, 0, 0), align, model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString());
	painter->setPen(QPen(palette().color(QPalette::Base)));
	painter->drawLine(rect.x(), rect.bottom(), rect.right(), rect.bottom());
}