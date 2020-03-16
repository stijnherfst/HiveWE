#include "UnitSingleModel.h"

#include <QPen>
#include <QPainter>
#include <QLineEdit>

#include "HiveWE.h"

UnitSingleModel::UnitSingleModel(QObject* parent) : QAbstractProxyModel(parent) {
	for (const auto& [key, index] : units_meta_slk.header_to_row) {
		if (units_slk.header_to_column.contains(to_lowercase_copy(units_meta_slk.data("field", key)))) {
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

	std::cout << sourceIndex.row() << "\t" << sourceIndex.column() << "\n";

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
			std::cout << "Found " << row << " at " << i << " " << headerData(i, Qt::Vertical, Qt::DisplayRole).toString().toStdString() << " " << units_meta_slk.data("displayname", id_mapping[i]) << "\n";
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

#include <QSpinBox>

TableDelegate::TableDelegate(QObject* parent) : QStyledItemDelegate(parent) {
}

QWidget* TableDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const {
	auto model = static_cast<const UnitSingleModel*>(index.model());
	auto& mapping = model->getMapping();

	std::string type = units_meta_slk.data("type", mapping[index.row()]);
	if (type == "int") {
		QSpinBox* editor = new QSpinBox(parent);
		std::string minVal = units_meta_slk.data("minval", mapping[index.row()]);
		std::string maxVal = units_meta_slk.data("maxval", mapping[index.row()]);
		// handle empty minVal, maxVal
		editor->setFrame(false);
		editor->setMinimum(std::stoi(minVal));
		editor->setMaximum(std::stoi(maxVal));
		return editor;
	}

	QLineEdit* editor = new QLineEdit(parent);
	return editor;
}

void TableDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
	//int value = index.model()->data(index, Qt::EditRole).toInt();

	//QSpinBox* spinBox = static_cast<QSpinBox*>(editor);
	//spinBox->setValue(value);
}

void TableDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
	//QSpinBox* spinBox = static_cast<QSpinBox*>(editor);
	//spinBox->interpretText();
	//int value = spinBox->value();

	//model->setData(index, value, Qt::EditRole);
}

void TableDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const {
	editor->setGeometry(option.rect);
}