#include "stdafx.h"

ImportManager::ImportManager(QWidget* parent) : QMainWindow(parent), ui(new Ui::ImportManager) {
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	ui->treeWidget->setFocus();
	ui->treeWidget->installEventFilter(this);
	ui->treeWidget->header()->resizeSection(0, 220);
	connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested, this, &ImportManager::custom_menu_popup);

	connect(ui->createDirectory, &QPushButton::clicked, [&]() { create_directory(ui->treeWidget->invisibleRootItem()); });
	connect(ui->importFiles, &QPushButton::clicked, [&]() { import_files(ui->treeWidget->invisibleRootItem()); });

	QFileIconProvider icons;
	folder_icon = icons.icon(QFileIconProvider::Folder);
	file_icon = icons.icon(QFileIconProvider::File);

	ui->createDirectory->setIcon(folder_icon);
	ui->importFiles->setIcon(file_icon);
	ui->exportAll->setIcon(file_icon);

	load_files(map.imports.imports);

	show();
}

void ImportManager::custom_menu_popup(const QPoint& pos) {
	QTreeWidgetItem* parent = ui->treeWidget->itemAt(pos);

	if (parent == nullptr) {
		parent = ui->treeWidget->invisibleRootItem();
	}

	QMenu* menu = new QMenu(this);
	QAction* import_files_action = new QAction("Import Files");
	QAction* remove_files_action = new QAction("Remove");
	QAction* export_file_action = new QAction("Export File");
	QAction* export_files_action = new QAction("Export Files");
	QAction* add_directory_action = new QAction("Add Directory");
	QAction* rename_action = new QAction("Rename");

	connect(import_files_action, &QAction::triggered, [&]() { import_files(parent); });
	connect(add_directory_action, &QAction::triggered, [&]() { create_directory(parent); });
	connect(remove_files_action, &QAction::triggered, [&]() { remove_item(parent);  });
	connect(rename_action, &QAction::triggered, [&]() { rename_directory(parent);  });
	connect(export_files_action, &QAction::triggered, [&]() { export_files(parent); });
	connect(export_file_action, &QAction::triggered, [&]() { export_files(parent); });

	if (parent->text(0).isEmpty() || parent->text(1) == "Directory") {
		menu->addAction(add_directory_action);
		menu->addAction(import_files_action);
		menu->addAction(export_files_action);
	} 
	
	if (parent->text(1) == "Directory") {
		menu->addAction(rename_action);
		menu->addAction(remove_files_action);
	} else if (!parent->text(0).isEmpty()) {
		menu->addAction(export_file_action);
		menu->addAction(remove_files_action);
	}

	menu->exec(QCursor::pos());
}

void ImportManager::import_files(QTreeWidgetItem* item) {
	QStringList files = QFileDialog::getOpenFileNames(this, "Select one or more files to import",
		".",
		"*.blp *.mdx *.mdl *.tga *.wav *.mp3 *.txt");

	for (auto&& file : files) {
		const QString file_name = QString::fromStdString(fs::path(file.toStdString()).filename().string());
		const QString full_path = "war3mapImported\\" + file_name;
		map.imports.import_file(file.toStdString(), file_name.toStdString());

		QTreeWidgetItem* child = new QTreeWidgetItem(item);
		child->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
		child->setText(0, file_name);
		child->setText(1, get_file_type(file.toStdString()));
		child->setText(2, QString::number(map.imports.file_size(file_name.toStdString())));
		child->setText(3, "No");
		child->setText(4, full_path);
		child->setIcon(0, file_icon);
	}
	item->setExpanded(true);
	items_changed();
}

void ImportManager::remove_item(QTreeWidgetItem* item) {
	const int choice = QMessageBox::warning(this, "HiveWE",
					"Are you sure you want to remove " + item->text(0),
					QMessageBox::Yes | QMessageBox::No);

	if (choice == QMessageBox::Yes) {
		delete item;

		hierarchy.map.file_remove(item->text(0).toStdString());
		items_changed();
	}
}

QString ImportManager::generate_full_path(QString file_name) {
	if (file_name.startsWith("BTN")) {
		return "ReplaceableTextures\\CommandButtons\\" + file_name;
	} else if (file_name.startsWith("PAS")) {
		return "ReplaceableTextures\\PassiveButtons\\" + file_name;
	} else if (file_name.startsWith("DISBTN") || file_name.startsWith("DISPAS")) {
		return "ReplaceableTextures\\CommandButtonsDisabled\\" + file_name;
	}

	return file_name;
}

QString ImportManager::get_file_type(const fs::path& path) {
	const std::string extension = path.extension().string();

	if (extension == ".blp" || extension == ".tga" ) {
		return "Image/Texture";
	} else if (extension == ".mdx") {
		return "Model";
	} else if (extension == ".txt") {
		return "Text";
	} else if (extension == ".mp3" || extension == ".wav") {
		return "Sound/Music";
	}

	return "Other";
}

void ImportManager::create_directory(QTreeWidgetItem* parent) {
	QTreeWidgetItem* item = new QTreeWidgetItem(parent);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
	item->setText(0, "Untitled Directory");
	item->setText(1, "Directory");
	item->setIcon(0, folder_icon);
	items_changed();
}

void ImportManager::rename_directory(QTreeWidgetItem* item) {
	QDialog * diag = new QDialog(this);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel);

	connect(buttons, &QDialogButtonBox::rejected, diag, &QDialog::reject);

	QLineEdit * input = new QLineEdit(diag);
	input->setPlaceholderText("Directory name...");
	input->setMaxLength(32);
	input->setText(item->text(0));

	connect(buttons, &QDialogButtonBox::accepted, [&]() {
		item->setText(0, input->text());
		diag->close();
	});
	connect(input, &QLineEdit::textChanged, [&](const QString &text) {
		buttons->button(QDialogButtonBox::Ok)->setDisabled(text.isEmpty());
	});

	mainLayout->addWidget(input);
	mainLayout->addWidget(buttons);

	diag->setLayout(mainLayout);
	diag->setWindowTitle("Rename Directory");
	diag->exec();
	items_changed();
}

void ImportManager::load_files(const std::vector<ImportItem>& imports) const {
	const std::function<void(std::vector<ImportItem>, QTreeWidgetItem*)> create_tree = [&](std::vector<ImportItem>& items, QTreeWidgetItem* parent) {
		for (auto&& i : items) {
			QTreeWidgetItem* item = new QTreeWidgetItem(parent);
			item->setText(0, QString::fromStdString(i.name.string()));
			if (i.children.size()) {
				item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
				item->setText(1, "Directory");
				item->setIcon(0, folder_icon);
				create_tree(i.children, item);
			} else {
				item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
				item->setText(1, get_file_type(i.name));
				item->setText(2, QString::number(i.size));
				item->setText(3, i.custom ? "Yes" : "No");
				item->setText(4, QString::fromStdString(i.full_path.string()));
				item->setIcon(0, file_icon);
			}
		}
	};

	create_tree(imports, ui->treeWidget->invisibleRootItem());
}

void ImportManager::export_files(QTreeWidgetItem* item) {
	const fs::path path = QFileDialog::getExistingDirectory(this, "Select Folder", ".").toStdString();

	const std::function<void(fs::path, QTreeWidgetItem*)> export_files = [&](fs::path target, QTreeWidgetItem* parent) {
		const int count = parent->childCount();
		
		if (count == 0) {
			map.imports.export_file(target, parent->text(4).toStdString());
		} else {
			target /= parent->text(0).toStdString();
			fs::create_directory(target);
			for (int i = 0; i < count; i++) {
				QTreeWidgetItem* child = parent->child(i);
				export_files(target, child);
			}
		}
	};

	export_files(path, item);
}

bool ImportManager::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QKeyEvent::KeyPress) {
		QKeyEvent* k_event = dynamic_cast<QKeyEvent *>(event);
		if (k_event->key() == Qt::Key_Delete) {
			remove_item(ui->treeWidget->currentItem());
			return true;
		}
	}
	if (event->type() == QEvent::ChildRemoved) {
		items_changed();
	}
	return false;
}

void ImportManager::items_changed() {
	map.imports.imports.clear();

	const std::function<void(std::vector<ImportItem>&, QTreeWidgetItem*)> recreate_imports = [&](std::vector<ImportItem>& items, QTreeWidgetItem* parent) {
		const int count = parent->childCount();
		for (int i = 0; i < count; i++) {
			QTreeWidgetItem* item = parent->child(i);
			ImportItem import_item;
			import_item.name = item->text(0).toStdString();
			import_item.size = item->text(2).toInt();
			import_item.custom = item->text(3) == "Yes";
			import_item.full_path = item->text(4).toStdString();

			if (item->childCount() > 0) {
				recreate_imports(import_item.children, item);
			}
			items.push_back(import_item);
		}
	};

	recreate_imports(map.imports.imports, ui->treeWidget->invisibleRootItem());
}