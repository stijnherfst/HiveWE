#include "SingleModel.h"

#include <QPen>
#include <QPainter>
#include <QLineEdit>
#include <QComboBox>
#include <QDialog>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QTimer>
#include <QSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QPushButton>

#include "UnitSelector.h"

#include "HiveWE.h"

SingleModel::SingleModel(slk::SLK2* slk, slk::SLK2* meta_slk, QObject* parent) : QAbstractProxyModel(parent), slk(slk), meta_slk(meta_slk) {
}

QModelIndex SingleModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}
	if (sourceIndex.row() != slk->row_headers.at(id)) {
		return {};
	}

	std::cout << sourceIndex.row() << "\t" << sourceIndex.column() << "\n";

	std::string meta_key;
	auto t = slk->index_to_column.at(sourceIndex.column());
	for (const auto& [key, value] : meta_slk->row_headers) {
		if (to_lowercase_copy(meta_slk->data("field", key)) == t) {
			meta_key = key;
			break;
		}
	}

	for (int i = 0; i < id_mapping.size(); i++) {
		if (id_mapping[i] == meta_key) {
			std::cout << "Found " << meta_key << " at " << i << " " << headerData(i, Qt::Vertical, Qt::DisplayRole).toString().toStdString() << " " << meta_slk->data("displayname", id_mapping[i]) << "\n";
			return createIndex(i, 0);
		}
	}
	return {};
}

QModelIndex SingleModel::mapToSource(const QModelIndex& proxyIndex) const {
	if (!proxyIndex.isValid()) {
		return {};
	}

	std::string column = to_lowercase_copy(meta_slk->data("field", id_mapping[proxyIndex.row()]));
	return sourceModel()->index(slk->row_headers.at(id), slk->column_headers.at(column));
}

QVariant SingleModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role != Qt::DisplayRole) {
		return {};
	}

	if (orientation == Qt::Orientation::Vertical) {
		std::string category = world_edit_data.data("ObjectEditorCategories", meta_slk->data("category", id_mapping[section]));
		category = string_replaced(category, "&", "");
		return QString::fromStdString(category + " - " + meta_slk->data("displayname", id_mapping[section]) + " (" + id_mapping[section] + ")");
	} else {
		return "UnitID";
	}
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

std::string SingleModel::getID() const {
	return id;
}

void SingleModel::setID(const std::string newID) {
	for (const auto& [key, index] : meta_slk->row_headers) {
		if (meta_slk->column_headers.contains("usespecific")) {
			if (!meta_slk->data("usespecific", key).empty() && meta_slk->data("usespecific", key).find(newID) == std::string::npos) {
				continue;
			}
		}

		// We add a virtual column if it does not exist in the base table
		if (!slk->column_headers.contains(to_lowercase_copy(meta_slk->data("field", key)))) {
			slk->add_column(to_lowercase_copy(meta_slk->data("field", key)));
		}

		id_mapping.push_back(key);
	}

	std::sort(id_mapping.begin(), id_mapping.end(), [&](const std::string& left, const std::string& right) {
		std::string category = world_edit_data.data("ObjectEditorCategories", meta_slk->data("category", left));
		category = string_replaced(category, "&", "");
		std::string left_string = category + " - " + meta_slk->data("displayname", left);

		category = world_edit_data.data("ObjectEditorCategories", meta_slk->data("category", right));
		category = string_replaced(category, "&", "");
		std::string right_string = category + " - " + meta_slk->data("displayname", right);

		return left_string < right_string;
	});

	beginResetModel();
	id = newID;
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

TableDelegate::TableDelegate(QWidget* parent) : QStyledItemDelegate(parent) {
}

QWidget* TableDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const {
	auto model = static_cast<const SingleModel*>(index.model());
	auto& mapping = model->getMapping();

	std::string type = model->meta_slk->data("type", mapping[index.row()]);
	std::string minVal = model->meta_slk->data("minval", mapping[index.row()]);
	std::string maxVal = model->meta_slk->data("maxval", mapping[index.row()]);

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
		QLineEdit* editor = new QLineEdit(parent);
//		editor->setMaxLength(std::stoi(maxVal));
		return editor;
	} else if (type == "targetList") {
		QDialog* dialog = new QDialog(parent, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
		dialog->setWindowModality(Qt::WindowModality::WindowModal);

		QVBoxLayout* layout = new QVBoxLayout(dialog);
		QGridLayout* flow = new QGridLayout;

		for (const auto& [key, value] : unit_editor_data.section(type)) {
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

		dialog->show();

		return dialog;
	} else if (type == "unitList") {
		QDialog* dialog = new QDialog(parent, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
		dialog->resize(256, 360);
		dialog->setWindowModality(Qt::WindowModality::WindowModal);

		QVBoxLayout* layout = new QVBoxLayout(dialog);


		QListWidget* list = new QListWidget;
		list->setObjectName("unitList");
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

			UnitSelector* selector = new UnitSelector(selectdialog);
			selectlayout->addWidget(selector);

			QDialogButtonBox* buttonBox2 = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
			connect(buttonBox2, &QDialogButtonBox::accepted, selectdialog, &QDialog::accept);
			connect(buttonBox2, &QDialogButtonBox::rejected, selectdialog, &QDialog::reject);
			selectlayout->addWidget(buttonBox2);

			connect(selector, &UnitSelector::unitSelected, [selectdialog, list](const std::string& id) {
				QListWidgetItem* item = new QListWidgetItem;
				item->setText(QString::fromStdString(units_slk.data("name", id)));
				item->setData(Qt::StatusTipRole, QString::fromStdString(id));
				auto one = units_slk.row_headers.at(id);
				auto two = units_slk.column_headers.at("art");
				item->setIcon(units_table->data(units_table->index(one, two), Qt::DecorationRole).value<QIcon>());
				list->addItem(item);
				selectdialog->close();
			});

			connect(selectdialog, &QDialog::accepted, selector, &UnitSelector::forceSelection);

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

		dialog->show();

		return dialog;
	} else if (type.ends_with("List")) {
		QDialog* dialog = new QDialog(parent, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
		dialog->setWindowModality(Qt::WindowModality::WindowModal);

		QVBoxLayout* layout = new QVBoxLayout(dialog);

		QPlainTextEdit* editor = new QPlainTextEdit;
		editor->setObjectName("editor");

		QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

		layout->addWidget(editor);
		layout->addWidget(buttonBox);

		dialog->show();

		return dialog;
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
	} else {
		return new QLineEdit(parent);
	}
}

void TableDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
	auto model = static_cast<const SingleModel*>(index.model());
	auto& mapping = model->getMapping();

	std::string type = model->meta_slk->data("type", mapping[index.row()]);

	if (type == "int") {
		static_cast<QSpinBox*>(editor)->setValue(model->data(index, Qt::EditRole).toInt());
	} else if (type == "real" || type == "unreal") {
		static_cast<QDoubleSpinBox*>(editor)->setValue(model->data(index, Qt::EditRole).toDouble());
	} else if (type == "string") {
		static_cast<QLineEdit*>(editor)->setText(model->data(index, Qt::EditRole).toString());
	} else if (type == "targetList") {
		auto parts = model->data(index, Qt::EditRole).toString().split(',');
		for (const auto& i : parts) {
			QCheckBox* box = editor->findChild<QCheckBox*>(i);
			if (box) {
				box->setChecked(true);
			}
		}

	} else if (type == "unitList") {
		QListWidget* list = editor->findChild<QListWidget*>("unitList");

		auto parts = model->data(index, Qt::EditRole).toString().split(',');
		for (const auto& i : parts) {
			QListWidgetItem* item = new QListWidgetItem;
			item->setText(QString::fromStdString(units_slk.data("name", i.toStdString())));
			item->setData(Qt::StatusTipRole, i);
			auto one = units_slk.row_headers.at(i.toStdString());
			auto two = units_slk.column_headers.at("art");
			item->setIcon(units_table->data(units_table->index(one, two), Qt::DecorationRole).value<QIcon>());
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
	} else {
		static_cast<QLineEdit*>(editor)->setText(model->data(index, Qt::EditRole).toString());
	}
}

void TableDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
	auto singlemodel = static_cast<SingleModel*>(model);
	auto& mapping = singlemodel->getMapping();

	std::string type = singlemodel->meta_slk->data("type", mapping[index.row()]);

	if (type == "int") {
		singlemodel->setData(index, static_cast<QSpinBox*>(editor)->value());
	} else if (type == "real" || type == "unreal") {
		singlemodel->setData(index, static_cast<QDoubleSpinBox*>(editor)->value());
	} else if (type == "string") {
		singlemodel->setData(index, static_cast<QLineEdit*>(editor)->text());
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