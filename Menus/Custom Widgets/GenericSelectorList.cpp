//#include "GenericSelectorList.h"
//
//#include <QLineEdit>
//#include <QComboBox>
//#include <QDialog>
//#include <QDialogButtonBox>
//#include <QListWidget>
//#include <QLayout>
//#include <QPushButton>
//
//GenericSelectorList::GenericSelectorList(QWidget* parent) : QDialog(parent) {
//	QDialog* dialog = new QDialog(parent, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
//	dialog->resize(256, 360);
//	dialog->setWindowModality(Qt::WindowModality::WindowModal);
//
//	QVBoxLayout* layout = new QVBoxLayout(dialog);
//
//	QListWidget* list = new QListWidget;
//	list->setObjectName("unitList");
//	list->setDragDropMode(QAbstractItemView::DragDropMode::InternalMove);
//	layout->addWidget(list);
//
//	QHBoxLayout* hbox = new QHBoxLayout;
//
//	QPushButton* add = new QPushButton("Add");
//	QPushButton* remove = new QPushButton("Remove");
//	remove->setDisabled(true);
//	hbox->addWidget(add);
//	hbox->addWidget(remove);
//	layout->addLayout(hbox);
//	connect(add, &QPushButton::clicked, [=]() {
//		QDialog* selectdialog = new QDialog(dialog, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
//		selectdialog->resize(300, 560);
//		selectdialog->setWindowModality(Qt::WindowModality::WindowModal);
//
//		QVBoxLayout* selectlayout = new QVBoxLayout(selectdialog);
//
//		UnitSelector* selector = new UnitSelector(selectdialog);
//		selectlayout->addWidget(selector);
//
//		QDialogButtonBox* buttonBox2 = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
//		connect(buttonBox2, &QDialogButtonBox::accepted, selectdialog, &QDialog::accept);
//		connect(buttonBox2, &QDialogButtonBox::rejected, selectdialog, &QDialog::reject);
//		selectlayout->addWidget(buttonBox2);
//
//		connect(selector, &UnitSelector::unitSelected, [selectdialog, list](const std::string& id) {
//			QListWidgetItem* item = new QListWidgetItem;
//			item->setText(QString::fromStdString(units_slk.data("name", id)));
//			item->setData(Qt::StatusTipRole, QString::fromStdString(id));
//			auto one = units_slk.row_headers.at(id);
//			auto two = units_slk.column_headers.at("art");
//			item->setIcon(units_table->data(units_table->index(one, two), Qt::DecorationRole).value<QIcon>());
//			list->addItem(item);
//			selectdialog->close();
//		});
//
//		connect(selectdialog, &QDialog::accepted, selector, &UnitSelector::forceSelection);
//
//		selectdialog->show();
//		selectdialog->move(dialog->geometry().topRight() + QPoint(10, dialog->geometry().height() - selectdialog->geometry().height()));
//	});
//	connect(remove, &QPushButton::clicked, [=]() {
//		for (auto i : list->selectedItems()) {
//			delete i;
//		}
//	});
//	connect(list, &QListWidget::itemSelectionChanged, [=]() {
//		remove->setEnabled(list->selectedItems().size() > 0);
//	});
//
//	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
//	connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
//	connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
//	layout->addWidget(buttonBox);
//
//	dialog->show();
//}