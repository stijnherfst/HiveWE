//module;
//
//#include <QObject>
//#include <QWidget>
//#include <QDialog>
//#include <QVBoxLayout>
//#include <QListWidget>
//#include <QPushButton>
//#include <QSortFilterProxyModel>
//#include <QTreeView>
//#include <QDialogButtonBox>
//#include <QTreeView>
//
export module ability_list_editor;
//
//import AbilityTreeModel;
//
//
//export QDialog* create_ability_list_editor(QWidget* editor) {
//	QDialog* dialog = new QDialog(editor, Qt::Window | Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
//	dialog->setObjectName("dialog");
//	dialog->resize(256, 360);
//	dialog->setWindowModality(Qt::WindowModality::WindowModal);
//
//	QVBoxLayout* layout = new QVBoxLayout(dialog);
//
//	QListWidget* list = new QListWidget;
//	list->setObjectName("abilityList");
//	list->setIconSize(QSize(32, 32));
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
//		AbilityTreeModel* abilityTreeModel = new AbilityTreeModel(dialog);
//		abilityTreeModel->setSourceModel(abilities_table);
//		QSortFilterProxyModel* filter = new QSortFilterProxyModel;
//		filter->setSourceModel(abilityTreeModel);
//		filter->setRecursiveFilteringEnabled(true);
//		filter->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
//
//		QLineEdit* search = new QLineEdit;
//		search->setPlaceholderText("Search Abilities");
//		QTreeView* view = new QTreeView;
//		view->setModel(filter);
//		view->header()->hide();
//		view->expandAll();
//
//		connect(search, &QLineEdit::textChanged, filter, QOverload<const QString&>::of(&QSortFilterProxyModel::setFilterFixedString));
//
//		selectlayout->addWidget(search);
//		selectlayout->addWidget(view);
//
//		QDialogButtonBox* buttonBox2 = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
//		connect(buttonBox2, &QDialogButtonBox::accepted, selectdialog, &QDialog::accept);
//		connect(buttonBox2, &QDialogButtonBox::rejected, selectdialog, &QDialog::reject);
//		selectlayout->addWidget(buttonBox2);
//
//		auto add = [filter, list, selectdialog](const QModelIndex& index) {
//			QModelIndex sourceIndex = filter->mapToSource(index);
//			BaseTreeItem* treeItem = static_cast<BaseTreeItem*>(sourceIndex.internalPointer());
//			if (treeItem->baseCategory || treeItem->subCategory) {
//				return;
//			}
//
//			std::print("Valid\n");
//
//			QListWidgetItem* item = new QListWidgetItem;
//			item->setText(QString::fromStdString(abilities_slk.data("name", treeItem->id)));
//			item->setData(Qt::StatusTipRole, QString::fromStdString(treeItem->id));
//			auto one = abilities_slk.row_headers.at(treeItem->id);
//			auto two = abilities_slk.column_headers.at("art");
//			item->setIcon(abilities_table->data(abilities_table->index(one, two), Qt::DecorationRole).value<QIcon>());
//			list->addItem(item);
//			selectdialog->close();
//		};
//
//		connect(view, &QTreeView::activated, [=](const QModelIndex& index) {
//			add(index);
//		});
//
//		connect(selectdialog, &QDialog::accepted, [=]() {
//			auto indices = view->selectionModel()->selectedIndexes();
//			if (indices.size()) {
//				add(indices.front());
//			}
//		});
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
//	// connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
//	connect(buttonBox, &QDialogButtonBox::accepted, [=]() {
//		dialog->accept();
//
//		auto yeet = const_cast<TableDelegate*>(this);
//		emit yeet->commitData(editor);
//		emit yeet->closeEditor(editor);
//	});
//	connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
//	layout->addWidget(buttonBox);
//
//	dialog->show();
//	// dialog->move(parent->geometry().center() - QPoint(dialog->geometry().width() / 2, dialog->geometry().height() / 2));
//	return editor;
//}