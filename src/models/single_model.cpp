#include "single_model.h"

#include "model_view.h"

#include <QPainter>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QTimer>
#include <QSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSortFilterProxyModel>

#include "object_editor/icon_view.h"

import std;
import BaseTreeModel;
import AbilityTreeModel;
import UpgradeTreeModel;
import UnitTreeModel;
import UnitSelector;
import Utilities;
import Globals;

SingleModel::SingleModel(TableModel* table, QObject* parent) : QAbstractProxyModel(parent) {
	slk = table->slk;
	meta_slk = table->meta_slk;
	setSourceModel(table);
	connect(this, &SingleModel::dataChanged, [this](const auto& index) {
		if (!index.isValid()) {
			return;
		}
		if (id_mapping[index.row()].field == "levels" || id_mapping[index.row()].field == "maxlevel" || id_mapping[index.row()].field == "numvar") {
			buildMapping();
		}
	});
}

QModelIndex SingleModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}

	if (sourceIndex.row() != slk->row_headers.at(id)) {
		std::print("Invalid ID for SLK {}\n", id);
		return {};
	}

	const std::string& field = slk->index_to_column.at(sourceIndex.column());
	for (size_t i = 0; i < id_mapping.size(); i++) {
		if (id_mapping[i].field == field) {
			//std::print("Found {} at {} {} {}\n", field, i, headerData(i, Qt::Vertical, Qt::DisplayRole).toString().toStdString(), meta_slk->data("displayname", id_mapping[i].key));
			return createIndex(i, 0);
		}
	}
	std::print("Not found {}\t{}\n", sourceIndex.row(), sourceIndex.column());
	return {};
}

QModelIndex SingleModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	const std::string& column = id_mapping[proxyIndex.row()].field;
	return sourceModel()->index(slk->row_headers.at(id), slk->column_headers.at(column));
}

QVariant SingleModel::data(const QModelIndex& index, int role) const {
	if (role != Qt::ForegroundRole) {
		return QAbstractProxyModel::data(index, role);
	}

	if (slk->shadow_data.contains(id) && slk->shadow_data.at(id).contains(id_mapping[index.row()].field)) {
		return QColor("violet");
	}

	return {};
}

QVariant SingleModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Orientation::Vertical) {
			const std::string_view category = world_edit_data.data<std::string_view>("ObjectEditorCategories", meta_slk->data<std::string_view>("category", id_mapping[section].key));
			const auto category2 = string_replaced(category, "&", "");
			const std::string_view display_name = meta_slk->data<std::string_view>("displayname", id_mapping[section].key);

			if (id_mapping[section].level > 0) {
				return QString::fromStdString(std::format("{} - {} - Level {} ({})", category2, display_name, id_mapping[section].level, id_mapping[section].key));
			} else {
				return QString::fromStdString(std::format("{} - {} ({})", category2, display_name, id_mapping[section].key));
			}
		} else {
			return "UnitID";
		}
	} else if (role == Qt::ForegroundRole) {
		if (orientation == Qt::Orientation::Vertical) {
			if (slk->shadow_data.contains(id) && slk->shadow_data.at(id).contains(id_mapping[section].field)) {
				return QColor("violet");
			} else {
				return QColor("white");
			}
		}
	}
	return QAbstractProxyModel::headerData(section, orientation, role);
}

bool SingleModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	// Force a header redraw because the header text should change color depending on whether the data field is a custom value or not
	emit headerDataChanged(Qt::Orientation::Vertical, index.row(), index.row());
	return QAbstractProxyModel::setData(index, value, role);
}

int SingleModel::rowCount(const QModelIndex& parent) const {
	return id_mapping.size();
}

int SingleModel::columnCount(const QModelIndex& parent) const {
	return 1;
}

QModelIndex SingleModel::index(int row, int column, const QModelIndex& parent) const {
	return createIndex(row, column);
}

QModelIndex SingleModel::parent(const QModelIndex& child) const {
	return QModelIndex();
}

void SingleModel::setSourceModel(QAbstractItemModel* sourceModel) {
	beginResetModel();

	if (this->sourceModel()) {
		disconnect(sourceModel, &QAbstractItemModel::dataChanged, this, &SingleModel::sourceDataChanged);
	}

	QAbstractProxyModel::setSourceModel(sourceModel);

	connect(sourceModel, &QAbstractItemModel::dataChanged, this, &SingleModel::sourceDataChanged);

	endResetModel();
}

std::string SingleModel::getID() const {
	return id;
}

void SingleModel::setID(const std::string newID) {
	id = newID;
	buildMapping();
}

void SingleModel::buildMapping() {
	beginResetModel();
	id_mapping.clear();

	for (const auto& [key, index] : meta_slk->row_headers) {
		// Unit an ability SLKs contain usespecific, but only abilities use `usespecific`, the cell is always empty for units.
		const std::string_view use_specific = meta_slk->data<std::string_view>("usespecific", key);
		const std::string_view not_specific = meta_slk->data<std::string_view>("notspecific", key);
		if (!use_specific.empty()) {
			std::string_view id_to_check = id;
			if (slk->shadow_data.contains("id") && slk->shadow_data.at(id).contains("oldid")) {
				id_to_check = slk->shadow_data.at(id).at("oldid");
			}

			// For abilities we also have to check the one this might be aliasing
			const std::string_view aliasing_ability = slk->data<std::string_view>("code", id);
			if (!use_specific.contains(id_to_check) && !use_specific.contains(aliasing_ability)) {
				continue;
			}
		}

		if (!not_specific.empty()) {
			std::string_view id_to_check = id;
			if (slk->shadow_data.contains("id") && slk->shadow_data.at(id).contains("oldid")) {
				id_to_check = slk->shadow_data.at(id).at("oldid");
			}

			// For abilities we also have to check the one this might be aliasing
			const std::string_view aliasing_ability = slk->data<std::string_view>("code", id);
			if (not_specific.contains(id_to_check) && not_specific.contains(aliasing_ability)) {
				continue;
			}
		}

		std::string field_name = to_lowercase_copy(meta_slk->data<std::string_view>("field", key));
		if (meta_slk->column_headers.contains("data") && meta_slk->data<int>("data", key) > 0) {
			field_name += static_cast<char>('a' + (meta_slk->data<int>("data", key) - 1));
		}

		if (meta_slk->column_headers.contains("repeat") && meta_slk->data<int>("repeat", key) > 0) {
			int iterations = 1;
			if (slk->column_headers.contains("levels")) {
				iterations = slk->data<int>("levels", id);
			} else if (slk->column_headers.contains("numvar")) {
				iterations = slk->data<int>("numvar", id);
			} else if (slk->column_headers.contains("maxlevel")) {
				iterations = slk->data<int>("maxlevel", id);
			}

			for (int i = 0; i < iterations; i++) {
				std::string new_field_name;
				if (meta_slk->column_headers.contains("appendindex") && meta_slk->data<int>("appendindex", key) > 0) {
					if (i == 0) {
						new_field_name = field_name;
					} else {
						new_field_name = field_name + std::to_string(i);
					}
				} else {
					new_field_name = field_name + std::to_string(i + 1);
				}

				// We add a virtual column if it does not exist in the base table
				if (!slk->column_headers.contains(new_field_name)) {
					slk->add_column(new_field_name);
				}

				id_mapping.push_back({ key, new_field_name, i + 1 });
			}
		} else {
			// We add a virtual column if it does not exist in the base table
			if (!slk->column_headers.contains(field_name)) {
				slk->add_column(field_name);
			}

			id_mapping.push_back({ key, field_name, 0 });
		}
	}

	std::ranges::sort(id_mapping, [&](const auto& left, const auto& right) {
		const std::string_view category1 = world_edit_data.data<std::string_view>("ObjectEditorCategories", meta_slk->data<std::string_view>("category", left.key));
		const std::string left_string = std::format("{} - {}{}", category1, meta_slk->data<std::string_view>("displayname", left.key), left.field);

		const std::string_view category2 = world_edit_data.data<std::string_view>("ObjectEditorCategories", meta_slk->data<std::string_view>("category", right.key));
		const std::string right_string = std::format("{} - {}{}", category2, meta_slk->data<std::string_view>("displayname", right.key), right.field);

		return left_string < right_string;
	});
	endResetModel();
}

void SingleModel::sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
	Q_ASSERT(topLeft.isValid() ? topLeft.model() == sourceModel() : true);
	Q_ASSERT(bottomRight.isValid() ? bottomRight.model() == sourceModel() : true);

	for (size_t i = topLeft.row(); i < bottomRight.row(); i++) {
		if (i == slk->row_headers.at(id)) {
			const auto top_left = mapFromSource(createIndex(i, topLeft.column()));
			const auto bottom_right = mapFromSource(createIndex(i, bottomRight.column()));
			emit dataChanged(top_left, bottom_right, roles);
			return;
		}
	}
}

/// Manually color the headers because the default QHeaderView will only alternatively color the items
void AlterHeader::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const {
	const Qt::Alignment align = (Qt::AlignLeft | Qt::AlignTop);

	if (logicalIndex % 2 > 0) {
		painter->fillRect(rect, palette().color(QPalette::AlternateBase));
	} else {
		painter->fillRect(rect, palette().color(QPalette::Base));
	}

	painter->setPen(QPen(model()->headerData(logicalIndex, orientation(), Qt::ForegroundRole).value<QColor>()));
	painter->drawText(rect.adjusted(2 * style()->pixelMetric(QStyle::PM_HeaderMargin, nullptr, this), 5, 0, 0), align, model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString());
	painter->setPen(QPen(palette().color(QPalette::Base)));
	painter->drawLine(rect.x(), rect.bottom(), rect.right(), rect.bottom());
}

TableDelegate::TableDelegate(QWidget* parent) : QStyledItemDelegate(parent) {
}

QWidget* TableDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const {
	const auto model = static_cast<const SingleModel*>(index.model());
	const auto& mapping = model->getMapping();

	const std::string type = model->meta_slk->data("type", mapping[index.row()].key);
	const std::string minVal = model->meta_slk->data("minval", mapping[index.row()].key);
	const std::string maxVal = model->meta_slk->data("maxval", mapping[index.row()].key);

	if (type == "int") {
		QSpinBox* editor = new QSpinBox(parent);

		// handle empty minVal, maxVal
		editor->setMinimum(std::stoi(minVal));
		editor->setMaximum(std::stoi(maxVal));
		editor->setSingleStep(std::clamp((std::stoi(maxVal) - std::stoi(minVal)) / 10, 1, 10));
		return editor;
	} else if (type == "real" || type == "unreal") {
		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setMinimum(std::stod(minVal));
		editor->setMaximum(std::stod(maxVal));
		editor->setSingleStep(std::clamp((std::stod(maxVal) - std::stod(minVal)) / 10.0, 0.1, 10.0));
		return editor;
	} else if (type == "string") {
		QTextEdit* editor = new QTextEdit(parent);
//		editor->setMaxLength(std::stoi(maxVal));
		return editor;
	} else if (type == "model") {
		return create_model_editor(parent);
	} else if (type == "targetList") {
		return create_target_list_editor(parent);
	} else if (type == "unitList") {
		return create_unit_list_editor(parent);
	} else if (type == "upgradeList") {
		return create_upgrade_list_editor(parent);
	}else if (type == "abilityList" || type == "heroAbilityList" || type == "abilitySkinList") {
		return create_ability_list_editor(parent);
	} else if (type.ends_with("List")) {
		return create_list_editor(parent);
	} else if (unit_editor_data.section_exists(type)) {
		QComboBox* editor = new QComboBox(parent);
		for (const auto& [key, value] : unit_editor_data.section(type)) {
			if (key == "NumValues" || key == "Sort" || key.ends_with("_Alt")) {
				continue;
			}

			QString displayText = QString::fromStdString(value[1]);
			displayText.replace('&', "");

			editor->addItem(displayText, QString::fromStdString(value[0]));
		}
		return editor;
	} else if (type == "icon") {
		return create_icon_editor(parent);
	} else if (type == "doodadCategory") {
		QComboBox* editor = new QComboBox(parent);
		for (auto&& [key, value] : world_edit_data.section("DoodadCategories")) {
			editor->addItem(QString::fromStdString(value[0]), QString::fromStdString(key));
		}
		return editor;
	} else if (type == "destructableCategory") {
		QComboBox* editor = new QComboBox(parent);
		for (auto&& [key, value] : world_edit_data.section("DestructibleCategories")) {
			editor->addItem(QString::fromStdString(value[0]), QString::fromStdString(key));
		}
		return editor;
	} else {
		return new QLineEdit(parent);
	}
}

void TableDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
	const auto model = static_cast<const SingleModel*>(index.model());
	const auto& mapping = model->getMapping();

	const std::string_view type = model->meta_slk->data<std::string_view>("type", mapping[index.row()].key);

	if (type == "int") {
		static_cast<QSpinBox*>(editor)->setValue(model->data(index, Qt::EditRole).toInt());
	} else if (type == "real" || type == "unreal") {
		static_cast<QDoubleSpinBox*>(editor)->setValue(model->data(index, Qt::EditRole).toDouble());
	} else if (type == "string") {
		// A hack to resolve TRIGSTR. The downside of taking the Display value is that we overwrite the TRIGSTR reference
		static_cast<QTextEdit*>(editor)->setText(model->data(index, Qt::DisplayRole).toString());
	} else if (type == "targetList") {
		auto parts = model->data(index, Qt::EditRole).toString().split(',');
		for (const auto& i : parts) {
			QCheckBox* box = editor->findChild<QCheckBox*>(i);
			if (box) {
				box->setChecked(true);
			}
		}
	} else if (type == "model") {
		ModelView* list = editor->findChild<ModelView*>("modelView");
		list->set_current_model_path(model->data(index, Qt::EditRole).toString());
	} else if (type == "unitList") {
		QListWidget* list = editor->findChild<QListWidget*>("unitList");

		auto ids = model->data(index, Qt::EditRole).toString().split(',', Qt::SkipEmptyParts);
		for (const auto& id : ids) {
			QListWidgetItem* item = new QListWidgetItem;
			item->setText(units_table->data(id.toStdString(), "name").toString());
			item->setIcon(units_table->data(id.toStdString(), "art", Qt::DecorationRole).value<QIcon>());
			item->setData(Qt::StatusTipRole, id);
			list->addItem(item);
		}
	} else if (type == "upgradeList") {
		QListWidget* list = editor->findChild<QListWidget*>("upgradeList");

		auto ids = model->data(index, Qt::EditRole).toString().split(',', Qt::SkipEmptyParts);
		for (const auto& id : ids) {
			QListWidgetItem* item = new QListWidgetItem;
			item->setText(upgrade_table->data(id.toStdString(), "name1").toString());
			item->setIcon(upgrade_table->data(id.toStdString(), "art1", Qt::DecorationRole).value<QIcon>());
			item->setData(Qt::StatusTipRole, id);
			list->addItem(item);
		}
	} else if (type == "abilityList" || type == "heroAbilityList" || type == "abilitySkinList") {
		QListWidget* list = editor->findChild<QListWidget*>("abilityList");

		auto ids = model->data(index, Qt::EditRole).toString().split(',', Qt::SkipEmptyParts);
		for (const auto& id : ids) {
			QListWidgetItem* item = new QListWidgetItem;
			item->setText(abilities_table->data(id.toStdString(), "name").toString());
			item->setIcon(abilities_table->data(id.toStdString(), "art", Qt::DecorationRole).value<QIcon>());
			item->setData(Qt::StatusTipRole, id);
			list->addItem(item);
		}
	} else if (type.ends_with("List")) {
		editor->findChild<QPlainTextEdit*>("editor")->setPlainText(model->data(index, Qt::EditRole).toString());
	} else if (unit_editor_data.section_exists(type)) {
		auto combo = static_cast<QComboBox*>(editor);
		// Find the item with the right userdata and set it as current index
		for (int i = 0; i < combo->count(); i++) {
			if (combo->itemData(i, Qt::UserRole).toString() == model->data(index, Qt::EditRole).toString()) {
				combo->setCurrentIndex(i);
			}
		}
	} else if (type == "icon") {
		IconView* list = editor->findChild<IconView*>("iconView");
		list->setCurrentIconPath(model->data(index, Qt::EditRole).toString());
	} else if (type == "doodadCategory") {
		const auto combo = static_cast<QComboBox*>(editor);
		for (int i = 0; i < combo->count(); i++) {
			if (combo->itemData(i, Qt::UserRole).toString() == model->data(index, Qt::EditRole).toString()) {
				combo->setCurrentIndex(i);
			}
		}
	} else if (type == "destructableCategory") {
		auto combo = static_cast<QComboBox*>(editor);
		for (int i = 0; i < combo->count(); i++) {
			if (combo->itemData(i, Qt::UserRole).toString() == model->data(index, Qt::EditRole).toString()) {
				combo->setCurrentIndex(i);
			}
		}
	} else {
		static_cast<QLineEdit*>(editor)->setText(model->data(index, Qt::EditRole).toString());
	}
}

void TableDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
	auto singlemodel = static_cast<SingleModel*>(model);
	auto& mapping = singlemodel->getMapping();

	const std::string_view type = singlemodel->meta_slk->data<std::string_view>("type", mapping[index.row()].key);

	if (type == "int") {
		singlemodel->setData(index, static_cast<QSpinBox*>(editor)->value());
	} else if (type == "real" || type == "unreal") {
		singlemodel->setData(index, static_cast<QDoubleSpinBox*>(editor)->value());
	} else if (type == "string") {
		auto text = static_cast<QTextEdit*>(editor)->toPlainText();
		text.replace('\n', "|n");
		singlemodel->setData(index, text);
	} else if (type == "model") {
		ModelView* list = editor->findChild<ModelView*>("modelView");
		singlemodel->setData(index, list->current_model_path());
	} else if (type == "unitList") {
		QListWidget* list = editor->findChild<QListWidget*>("unitList");

		QString result;
		for (int i = 0; i < list->count(); i++) {
			QListWidgetItem* item = list->item(i);
			if (!result.isEmpty()) {
				result += ',';
			}
			result += item->data(Qt::StatusTipRole).toString();
		}
		model->setData(index, result, Qt::EditRole);
	} else if (type == "abilityList" || type == "heroAbilityList" || type == "abilitySkinList") {
		QListWidget* list = editor->findChild<QListWidget*>("abilityList");
		
		QString result;
		for (int i = 0; i < list->count(); i++) {
			QListWidgetItem* item = list->item(i);
			if (!result.isEmpty()) {
				result += ',';
			}
			result += item->data(Qt::StatusTipRole).toString();
		}
		model->setData(index, result, Qt::EditRole);
	} else if (type == "upgradeList") {
		QListWidget* list = editor->findChild<QListWidget*>("upgradeList");

		QString result;
		for (int i = 0; i < list->count(); i++) {
			QListWidgetItem* item = list->item(i);
			if (!result.isEmpty()) {
				result += ',';
			}
			result += item->data(Qt::StatusTipRole).toString();
		}
		model->setData(index, result, Qt::EditRole);
	} else if (type == "targetList") {
		QString result;
		for (const auto& [key, value] : unit_editor_data.section(type)) {
			if (key == "NumValues" || key == "Sort" || key.ends_with("_Alt")) {
				continue;
			}

			QCheckBox* box = editor->findChild<QCheckBox*>(QString::fromStdString(value[0]));
			if (box && box->isChecked()) {
				if (!result.isEmpty()) {
					result += ",";
				}
				result += QString::fromStdString(value[0]);
			}
		}
		model->setData(index, result, Qt::EditRole);
	}  else if (type.ends_with("List")) {
		singlemodel->setData(index, editor->findChild<QPlainTextEdit*>("editor")->toPlainText());
	} else if (unit_editor_data.section_exists(type)) {
		auto combo = static_cast<QComboBox*>(editor);
		singlemodel->setData(index, combo->currentData());
	} else if (type == "icon") {
		IconView* list = editor->findChild<IconView*>("iconView");
		singlemodel->setData(index, list->currentIconPath());
	} else if (type == "doodadCategory") {
		auto combo = static_cast<QComboBox*>(editor);
		singlemodel->setData(index, combo->currentData());
	} else if (type == "destructableCategory") {
		auto combo = static_cast<QComboBox*>(editor);
		singlemodel->setData(index, combo->currentData());
	} else {
		singlemodel->setData(index, static_cast<QLineEdit*>(editor)->text());
	}
}

void TableDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const {
	if (dynamic_cast<QDialog*>(editor)) {
		
	} else {
		editor->setGeometry(option.rect);
	}
}

QWidget* TableDelegate::create_list_editor(QWidget* parent) const {
	auto editor = new QWidget(parent);

	QDialog* dialog = new QDialog(editor, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowModality(Qt::WindowModality::WindowModal);

	QVBoxLayout* layout = new QVBoxLayout(dialog);

	QPlainTextEdit* text_edit = new QPlainTextEdit;
	text_edit->setObjectName("editor");

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

	layout->addWidget(text_edit);
	layout->addWidget(buttonBox);

	connect(dialog, &QDialog::accepted, [=]() {
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->commitData(editor);
		emit delegate->closeEditor(editor);
	});

	connect(dialog, &QDialog::rejected, [=]() {
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->closeEditor(editor);
	});

	dialog->show();

	return editor;

}

QWidget* TableDelegate::create_model_editor(QWidget* parent) const {
	auto editor = new QWidget(parent);

	QDialog* dialog = new QDialog(editor, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->resize(530, 512);
	dialog->setWindowModality(Qt::WindowModality::WindowModal);

	ModelView* view = new ModelView;
	view->setObjectName("modelView");

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

	connect(dialog, &QDialog::accepted, [=]() {
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->commitData(editor);
		emit delegate->closeEditor(editor);
	});

	connect(dialog, &QDialog::rejected, [=]() {
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->closeEditor(editor);
	});

	QVBoxLayout* layout = new QVBoxLayout(dialog);
	layout->addWidget(view);
	layout->addWidget(buttonBox);

	dialog->show();

	return editor;
}


QWidget* TableDelegate::create_target_list_editor(QWidget* parent) const {
	auto editor = new QWidget(parent);

	QDialog* dialog = new QDialog(editor, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowModality(Qt::WindowModality::WindowModal);

	QVBoxLayout* layout = new QVBoxLayout(dialog);
	QGridLayout* flow = new QGridLayout;

	for (const auto& [key, value] : unit_editor_data.section("targetList")) {
		if (key == "NumValues" || key == "Sort" || key.ends_with("_Alt")) {
			continue;
		}

		QString displayText = QString::fromStdString(value[1]);
		displayText.replace('&', "");

		QCheckBox* flag = new QCheckBox(displayText);
		flag->setObjectName(QString::fromStdString(value[0]));

		flow->addWidget(flag);
	}

	QDialogButtonBox* buttonBox = new QDialogButtonBox;
	buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

	layout->addLayout(flow);
	layout->addWidget(buttonBox);

	connect(dialog, &QDialog::accepted, [=]() {
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->commitData(editor);
		emit delegate->closeEditor(editor);
	});

	connect(dialog, &QDialog::rejected, [=]() {
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->closeEditor(editor);
	});

	dialog->show();

	return editor;
}

QWidget* TableDelegate::create_upgrade_list_editor(QWidget* parent) const {
	auto editor = new QWidget(parent);

	QDialog* dialog = new QDialog(editor, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->resize(256, 360);
	dialog->setWindowModality(Qt::WindowModality::WindowModal);

	QVBoxLayout* layout = new QVBoxLayout(dialog);

	QListWidget* list = new QListWidget;
	list->setObjectName("upgradeList");
	list->setIconSize(QSize(32, 32));
	list->setDragDropMode(QAbstractItemView::DragDropMode::InternalMove);
	layout->addWidget(list);

	QHBoxLayout* hbox = new QHBoxLayout;

	QPushButton* add = new QPushButton("Add");
	QPushButton* remove = new QPushButton("Remove");
	remove->setDisabled(true);
	hbox->addWidget(add);
	hbox->addWidget(remove);
	layout->addLayout(hbox);
	connect(add, &QPushButton::clicked, [=]() {
		QDialog* selectdialog = new QDialog(dialog, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
		selectdialog->resize(300, 560);
		selectdialog->setWindowModality(Qt::WindowModality::WindowModal);

		QVBoxLayout* selectlayout = new QVBoxLayout(selectdialog);

		UpgradeTreeModel* uupgradeTreeModel = new UpgradeTreeModel(dialog);
		uupgradeTreeModel->setSourceModel(upgrade_table);
		QSortFilterProxyModel* filter = new QSortFilterProxyModel;
		filter->setSourceModel(uupgradeTreeModel);
		filter->setRecursiveFilteringEnabled(true);
		filter->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

		QLineEdit* search = new QLineEdit;
		search->setPlaceholderText("Search Upgrades");
		QTreeView* view = new QTreeView;
		view->setModel(filter);
		view->header()->hide();
		view->setSelectionBehavior(QAbstractItemView::SelectRows);
		view->setSelectionMode(QAbstractItemView::ExtendedSelection);
		view->expandAll();

		connect(search, &QLineEdit::textChanged, filter, QOverload<const QString&>::of(&QSortFilterProxyModel::setFilterFixedString));

		selectlayout->addWidget(search);
		selectlayout->addWidget(view);

		QDialogButtonBox* buttonBox2 = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		connect(buttonBox2, &QDialogButtonBox::accepted, selectdialog, &QDialog::accept);
		connect(buttonBox2, &QDialogButtonBox::rejected, selectdialog, &QDialog::reject);
		selectlayout->addWidget(buttonBox2);

		auto add = [filter, list, selectdialog](const QModelIndex& index) {
			QModelIndex sourceIndex = filter->mapToSource(index);
			BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(sourceIndex.internalPointer());
			if (treeItem->baseCategory || treeItem->subCategory) {
				return;
			}

			QListWidgetItem* item = new QListWidgetItem;
			item->setData(Qt::StatusTipRole, QString::fromStdString(treeItem->id));
			item->setText(upgrade_table->data(treeItem->id, "name1").toString());
			item->setIcon(upgrade_table->data(treeItem->id, "art1", Qt::DecorationRole).value<QIcon>());
			list->addItem(item);
		};

		connect(view, &QTreeView::activated, [=](const QModelIndex& index) {
			add(index);
			selectdialog->close();
		});

		connect(selectdialog, &QDialog::accepted, [=]() {
			for (const auto& i : view->selectionModel()->selectedIndexes()) {
				add(i);
			}
			selectdialog->close();
		});

		selectdialog->show();
		selectdialog->move(dialog->geometry().topRight() + QPoint(10, dialog->geometry().height() - selectdialog->geometry().height()));
	});

	connect(remove, &QPushButton::clicked, [=]() {
		for (auto i : list->selectedItems()) {
			delete i;
		}
	});
	connect(list, &QListWidget::itemSelectionChanged, [=]() {
		remove->setEnabled(list->selectedItems().size() > 0);
	});

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
	layout->addWidget(buttonBox);

	connect(dialog, &QDialog::accepted, [=](){
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->commitData(editor);
		emit delegate->closeEditor(editor);
	});

	connect(dialog, &QDialog::rejected, [=]() {
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->closeEditor(editor);
	});

	dialog->show();

	return editor;
}

QWidget* TableDelegate::create_unit_list_editor(QWidget* parent) const {
	auto editor = new QWidget(parent);

	QDialog* dialog = new QDialog(editor, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->resize(256, 360);
	dialog->setWindowModality(Qt::WindowModality::WindowModal);

	QVBoxLayout* layout = new QVBoxLayout(dialog);

	QListWidget* list = new QListWidget;
	list->setObjectName("unitList");
	list->setIconSize(QSize(32, 32));
	list->setDragDropMode(QAbstractItemView::DragDropMode::InternalMove);
	layout->addWidget(list);

	QHBoxLayout* hbox = new QHBoxLayout;

	QPushButton* add = new QPushButton("Add");
	QPushButton* remove = new QPushButton("Remove");
	remove->setDisabled(true);
	hbox->addWidget(add);
	hbox->addWidget(remove);
	layout->addLayout(hbox);
	connect(add, &QPushButton::clicked, [=]() {
		QDialog* selectdialog = new QDialog(dialog, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
		selectdialog->resize(300, 560);
		selectdialog->setWindowModality(Qt::WindowModality::WindowModal);

		QVBoxLayout* selectlayout = new QVBoxLayout(selectdialog);

		UnitTreeModel* unitTreeModel = new UnitTreeModel(dialog);
		unitTreeModel->setSourceModel(units_table);
		QSortFilterProxyModel* filter = new QSortFilterProxyModel;
		filter->setSourceModel(unitTreeModel);
		filter->setRecursiveFilteringEnabled(true);
		filter->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

		QLineEdit* search = new QLineEdit;
		search->setPlaceholderText("Search Units");
		QTreeView* view = new QTreeView;
		view->setModel(filter);
		view->header()->hide();
		view->setSelectionBehavior(QAbstractItemView::SelectRows);
		view->setSelectionMode(QAbstractItemView::ExtendedSelection);
		view->expandAll();

		connect(search, &QLineEdit::textChanged, filter, QOverload<const QString&>::of(&QSortFilterProxyModel::setFilterFixedString));

		selectlayout->addWidget(search);
		selectlayout->addWidget(view);

		QDialogButtonBox* buttonBox2 = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		connect(buttonBox2, &QDialogButtonBox::accepted, selectdialog, &QDialog::accept);
		connect(buttonBox2, &QDialogButtonBox::rejected, selectdialog, &QDialog::reject);
		selectlayout->addWidget(buttonBox2);

		auto add = [filter, list, selectdialog](const QModelIndex& index) {
			QModelIndex sourceIndex = filter->mapToSource(index);
			BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(sourceIndex.internalPointer());
			if (treeItem->baseCategory || treeItem->subCategory) {
				return;
			}

			QListWidgetItem* item = new QListWidgetItem;
			item->setData(Qt::StatusTipRole, QString::fromStdString(treeItem->id));
			item->setText(units_table->data(treeItem->id, "name").toString());
			item->setIcon(units_table->data(treeItem->id, "art", Qt::DecorationRole).value<QIcon>());
			list->addItem(item);
		};

		connect(view, &QTreeView::activated, [=](const QModelIndex& index) {
			add(index);
			selectdialog->close();
		});

		connect(selectdialog, &QDialog::accepted, [=]() {
			for (const auto& i : view->selectionModel()->selectedIndexes()) {
				add(i);
			}
			selectdialog->close();
		});

		selectdialog->show();
		selectdialog->move(dialog->geometry().topRight() + QPoint(10, dialog->geometry().height() - selectdialog->geometry().height()));
	});

	connect(remove, &QPushButton::clicked, [=]() {
		for (auto i : list->selectedItems()) {
			delete i;
		}
	});
	connect(list, &QListWidget::itemSelectionChanged, [=]() {
		remove->setEnabled(list->selectedItems().size() > 0);
	});

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
	layout->addWidget(buttonBox);

	connect(dialog, &QDialog::accepted, [=](){
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->commitData(editor);
		emit delegate->closeEditor(editor);
	});

	connect(dialog, &QDialog::rejected, [=]() {
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->closeEditor(editor);
	});

	dialog->show();

	return editor;
}

QWidget* TableDelegate::create_ability_list_editor(QWidget* parent) const {
	auto editor = new QWidget(parent);

	QDialog* dialog = new QDialog(editor, Qt::Window | Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->resize(256, 360);
	dialog->setWindowModality(Qt::WindowModality::WindowModal);

	QVBoxLayout* layout = new QVBoxLayout(dialog);

	QListWidget* list = new QListWidget;
	list->setObjectName("abilityList");
	list->setIconSize(QSize(32, 32));
	list->setDragDropMode(QAbstractItemView::DragDropMode::InternalMove);
	list->setSelectionBehavior(QAbstractItemView::SelectRows);
	list->setSelectionMode(QAbstractItemView::ExtendedSelection);

	layout->addWidget(list);

	QHBoxLayout* hbox = new QHBoxLayout;

	QPushButton* add = new QPushButton("Add");
	QPushButton* remove = new QPushButton("Remove");
	remove->setDisabled(true);
	hbox->addWidget(add);
	hbox->addWidget(remove);
	layout->addLayout(hbox);
	connect(add, &QPushButton::clicked, [=]() {
		QDialog* selectdialog = new QDialog(dialog, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
		selectdialog->resize(300, 560);
		selectdialog->setWindowModality(Qt::WindowModality::WindowModal);

		QVBoxLayout* selectlayout = new QVBoxLayout(selectdialog);

		AbilityTreeModel* abilityTreeModel = new AbilityTreeModel(dialog);
		abilityTreeModel->setSourceModel(abilities_table);
		QSortFilterProxyModel* filter = new QSortFilterProxyModel;
		filter->setSourceModel(abilityTreeModel);
		filter->setRecursiveFilteringEnabled(true);
		filter->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

		QLineEdit* search = new QLineEdit;
		search->setPlaceholderText("Search Abilities");
		QTreeView* view = new QTreeView;
		view->setModel(filter);
		view->header()->hide();
		view->setSelectionBehavior(QAbstractItemView::SelectRows);
		view->setSelectionMode(QAbstractItemView::ExtendedSelection);
		view->expandAll();

		connect(search, &QLineEdit::textChanged, filter, QOverload<const QString&>::of(&QSortFilterProxyModel::setFilterFixedString));

		selectlayout->addWidget(search);
		selectlayout->addWidget(view);

		QDialogButtonBox* buttonBox2 = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		connect(buttonBox2, &QDialogButtonBox::accepted, selectdialog, &QDialog::accept);
		connect(buttonBox2, &QDialogButtonBox::rejected, selectdialog, &QDialog::reject);
		selectlayout->addWidget(buttonBox2);

		auto add = [filter, list, selectdialog](const QModelIndex& index) {
			QModelIndex sourceIndex = filter->mapToSource(index);
			BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(sourceIndex.internalPointer());
			if (treeItem->baseCategory || treeItem->subCategory) {
				return;
			}

			QListWidgetItem* item = new QListWidgetItem;
			item->setData(Qt::StatusTipRole, QString::fromStdString(treeItem->id));
			item->setText(abilities_table->data(treeItem->id, "name").toString());
			item->setIcon(abilities_table->data(treeItem->id, "art", Qt::DecorationRole).value<QIcon>());

			list->addItem(item);
		};

		connect(view, &QTreeView::activated, [=](const QModelIndex& index) {
			add(index);
			selectdialog->close();
		});

		connect(selectdialog, &QDialog::accepted, [=]() {
			for (const auto& i : view->selectionModel()->selectedIndexes()) {
				add(i);
			}
			selectdialog->close();
		});

		selectdialog->show();
		selectdialog->move(dialog->geometry().topRight() + QPoint(10, dialog->geometry().height() - selectdialog->geometry().height()));
	});
	connect(remove, &QPushButton::clicked, [=]() {
		for (auto i : list->selectedItems()) {
			delete i;
		}
	});
	connect(list, &QListWidget::itemSelectionChanged, [=]() {
		remove->setEnabled(list->selectedItems().size() > 0);
	});

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
	layout->addWidget(buttonBox);

	connect(dialog, &QDialog::accepted, [=]() {
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->commitData(editor);
		emit delegate->closeEditor(editor);
	});

	connect(dialog, &QDialog::rejected, [=]() {
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->closeEditor(editor);
	});

	dialog->show();

	return editor;
}

QWidget* TableDelegate::create_icon_editor(QWidget* parent) const {
	auto editor = new QWidget(parent);

	QDialog* dialog = new QDialog(editor, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->resize(530, 512);
	dialog->setWindowModality(Qt::WindowModality::WindowModal);

	IconView* view = new IconView;
	view->setObjectName("iconView");

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

	connect(dialog, &QDialog::accepted, [=]() {
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->commitData(editor);
		emit delegate->closeEditor(editor);
	});

	connect(dialog, &QDialog::rejected, [=]() {
		const auto delegate = const_cast<TableDelegate*>(this);
		emit delegate->closeEditor(editor);
	});

	QVBoxLayout* layout = new QVBoxLayout(dialog);
	layout->addWidget(view);
	layout->addWidget(buttonBox);

	dialog->show();

	return editor;
}