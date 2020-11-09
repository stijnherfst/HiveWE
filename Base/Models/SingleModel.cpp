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
#include <QtreeView>

#include "UnitSelector.h"
#include "GenericSelectorList.h"
#include "IconView.h"
#include "AbilityTreeModel.h"

#include "HiveWE.h"

#include "fmt/format.h"

SingleModel::SingleModel(TableModel* table, QObject* parent) : QIdentityProxyModel(parent) {
	slk = table->slk;
	meta_slk = table->meta_slk;
	setSourceModel(table);
	connect(this, &SingleModel::dataChanged, [this](const auto& index) {
		if (id_mapping[index.row()].field == "levels" || id_mapping[index.row()].field == "numvar") {
			buildMapping();
		}
	});
}

QModelIndex SingleModel::mapFromSource(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return {};
	}
	if (sourceIndex.row() != slk->row_headers.at(id)) {
		fmt::print("Invalid ID for SLK {}\n", id);
		return {};
	}

	const std::string& field = slk->index_to_column.at(sourceIndex.column());
	for (int i = 0; i < id_mapping.size(); i++) {
		if (id_mapping[i].field == field) {
			fmt::print("Found {} at {} {} {}\n", field, i, headerData(i, Qt::Vertical, Qt::DisplayRole).toString().toStdString(), meta_slk->data("displayname", id_mapping[i].key));
			return createIndex(i, 0);
		}
	}
	fmt::print("Not found {}\t{}\n", sourceIndex.row(), sourceIndex.column());
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
	if (role != Qt::TextColorRole) {
		return QIdentityProxyModel::data(index, role);
	}

	if (slk->shadow_data.contains(id) && slk->shadow_data.at(id).contains(id_mapping[index.row()].field)) {
		return QColor("violet");
	}

	return {};
}

QVariant SingleModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role != Qt::DisplayRole) {
		return {};
	}

	if (orientation == Qt::Orientation::Vertical) {
		std::string category = world_edit_data.data("ObjectEditorCategories", meta_slk->data("category", id_mapping[section].key));
		category = string_replaced(category, "&", "");
		std::string display_name = meta_slk->data("displayname", id_mapping[section].key);

		int level = id_mapping[section].level;

		if (id_mapping[section].level > 0) {
			return QString::fromStdString(fmt::format("{} - {} - Level {} ({})", category, display_name, id_mapping[section].level, id_mapping[section].key));
		} else {
			return QString::fromStdString(fmt::format("{} - {} ({})", category, display_name, id_mapping[section].key));
		}
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
	id = newID;
	buildMapping();
}

void SingleModel::buildMapping() {
	beginResetModel();
	id_mapping.clear();

	for (const auto& [key, index] : meta_slk->row_headers) {
		if (meta_slk->column_headers.contains("usespecific")) {
			if (!meta_slk->data("usespecific", key).empty() && meta_slk->data("usespecific", key).find(id) == std::string::npos) {
				continue;
			}
		}

		std::string field_name = to_lowercase_copy(meta_slk->data("field", key));
		if (meta_slk->column_headers.contains("data") && meta_slk->data<int>("data", key) > 0) {
			field_name += static_cast<char>('a' + (meta_slk->data<int>("data", key) - 1));
		}

		if (meta_slk->column_headers.contains("repeat") && meta_slk->data<int>("repeat", key) > 0) {

			int iterations = 1;
			if (slk->column_headers.contains("levels")) {
				iterations = slk->data<int>("levels", id);
			} else if (slk->column_headers.contains("numvar")) {
				iterations = slk->data<int>("numvar", id);
			}

			for (int i = 0; i < iterations; i++) {
				std::string new_field_name = field_name + std::to_string(i + 1);

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

	std::sort(id_mapping.begin(), id_mapping.end(), [&](const auto& left, const auto& right) {
		std::string category = world_edit_data.data("ObjectEditorCategories", meta_slk->data("category", left.key));
		//category = string_replaced(category, "&", "");
		const std::string left_string = category + " - " + meta_slk->data("displayname", left.key) + left.field;

		category = world_edit_data.data("ObjectEditorCategories", meta_slk->data("category", right.key));
		//category = string_replaced(category, "&", "");
		const std::string right_string = category + " - " + meta_slk->data("displayname", right.key) + right.field;

		return left_string < right_string;
	});
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

// ToDo look into splitting/simplifying the following functions 
QWidget* TableDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const {
	auto model = static_cast<const SingleModel*>(index.model());
	auto& mapping = model->getMapping();

	std::string type = model->meta_slk->data("type", mapping[index.row()].key);
	std::string minVal = model->meta_slk->data("minval", mapping[index.row()].key);
	std::string maxVal = model->meta_slk->data("maxval", mapping[index.row()].key);

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
	} else if (type == "abilityList") {
		QDialog* dialog = new QDialog(parent, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
		dialog->resize(256, 360);
		dialog->setWindowModality(Qt::WindowModality::WindowModal);

		QVBoxLayout* layout = new QVBoxLayout(dialog);

		QListWidget* list = new QListWidget;
		list->setObjectName("abilityList");
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

			//UnitSelector* selector = new UnitSelector(selectdialog);

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
				if (sourceIndex.parent().isValid()) {
					if (sourceIndex.parent().parent().isValid()) {
						//std::cout << "valid\n";
						fmt::print("Valid\n");
						BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(sourceIndex.internalPointer());

						std::string id = abilities_slk.index_to_row.at(treeItem->tableRow);
						QListWidgetItem* item = new QListWidgetItem;
						item->setText(QString::fromStdString(abilities_slk.data("name", id)));
						item->setData(Qt::StatusTipRole, QString::fromStdString(id));
						auto one = abilities_slk.row_headers.at(id);
						auto two = abilities_slk.column_headers.at("art");
						item->setIcon(abilities_table->data(abilities_table->index(one, two), Qt::DecorationRole).value<QIcon>());
						list->addItem(item);
						selectdialog->close();
					}
				}
			};

			connect(view, &QTreeView::activated, [=](const QModelIndex& index) {
				add(index);
			});
			
			connect(selectdialog, &QDialog::accepted, [=]() {
				auto indices = view->selectionModel()->selectedIndexes();
				if (indices.size()) {
					add(indices.front());
				}
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
	} else if (type == "icon") {
		QDialog* dialog = new QDialog(parent, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
		dialog->resize(530, 512);
		dialog->setWindowModality(Qt::WindowModality::WindowModal);

		IconView* view = new IconView;
		view->setObjectName("iconView");

		QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

		QVBoxLayout* layout = new QVBoxLayout(dialog);
		layout->addWidget(view);
		layout->addWidget(buttonBox);

		dialog->show();
		return dialog;
	} else {
		return new QLineEdit(parent);
	}
}

void TableDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
	auto model = static_cast<const SingleModel*>(index.model());
	auto& mapping = model->getMapping();

	std::string type = model->meta_slk->data("type", mapping[index.row()].key);

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

		auto parts = model->data(index, Qt::EditRole).toString().split(',', QString::SkipEmptyParts);
		for (const auto& i : parts) {
			QListWidgetItem* item = new QListWidgetItem;
			item->setText(QString::fromStdString(units_slk.data("name", i.toStdString())));
			item->setData(Qt::StatusTipRole, i);
			auto one = units_slk.row_headers.at(i.toStdString());
			auto two = units_slk.column_headers.at("art");
			item->setIcon(units_table->data(units_table->index(one, two), Qt::DecorationRole).value<QIcon>());
			list->addItem(item);
		}
	} else if (type == "abilityList") {
		QListWidget* list = editor->findChild<QListWidget*>("abilityList");

		auto parts = model->data(index, Qt::EditRole).toString().split(',', QString::SkipEmptyParts);
		for (const auto& i : parts) {
			QListWidgetItem* item = new QListWidgetItem;
			item->setText(QString::fromStdString(abilities_slk.data("name", i.toStdString())));
			item->setData(Qt::StatusTipRole, i);
			auto one = abilities_slk.row_headers.at(i.toStdString());
			auto two = abilities_slk.column_headers.at("art");
			item->setIcon(abilities_table->data(abilities_table->index(one, two), Qt::DecorationRole).value<QIcon>());
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
	} else {
		static_cast<QLineEdit*>(editor)->setText(model->data(index, Qt::EditRole).toString());
	}
}

void TableDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
	auto singlemodel = static_cast<SingleModel*>(model);
	auto& mapping = singlemodel->getMapping();

	std::string type = singlemodel->meta_slk->data("type", mapping[index.row()].key);

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
	} else if (type == "abilityList") {
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