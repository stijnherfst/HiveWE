#include "stdafx.h"

ImportManager::ImportManager(QWidget *parent) : QMainWindow(parent), ui(new Ui::ImportManager) {
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->treeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

	ui->treeWidget->setFocusPolicy(Qt::ClickFocus);
	ui->treeWidget->setFocus();
	ui->treeWidget->installEventFilter(this);
	ui->treeWidget->setUniformRowHeights(true);
	ui->treeWidget->header()->resizeSection(0, 220);
	connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested, this, &ImportManager::custom_menu_popup);

	load_files(map.imports.directories, map.imports.imports);

	show();
}

ImportManager::~ImportManager() {
	delete ui;
}

QTreeWidgetItem * ImportManager::create_directory(const QString name) {
	QTreeWidgetItem* item = new QTreeWidgetItem();
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
	item->setText(0, name);
	item->setText(1, "Directory");

	return item;
}

void ImportManager::add_child_item(QTreeWidgetItem* itm, const QString name, const QString item_type, const int item_size, const QString full_path) const {
	QTreeWidgetItem * childitm = new QTreeWidgetItem();
	childitm->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
	childitm->setText(0, name);
	childitm->setText(1, item_type);
	childitm->setData(2, Qt::DisplayRole, item_size);
	childitm->setText(3, full_path);

	itm->addChild(childitm);
}

void ImportManager::custom_menu_popup(const QPoint & pos) {
	QTreeWidgetItem * itm = ui->treeWidget->itemAt(pos);

	QMenu* menu = new QMenu(this);
	QAction* import_files_action = new QAction("Import Files");
	QAction* remove_files_action = new QAction("Remove File");
	QAction* export_file_action = new QAction("Export File");
	QAction* export_files_action = new QAction("Export Files");
	QAction* add_directory_action = new QAction("Add Directory");
	QAction* rename_action = new QAction("Rename");

	connect(import_files_action, &QAction::triggered, [&]() { import_files(itm); });
	connect(add_directory_action, &QAction::triggered, [&]() { ui->treeWidget->addTopLevelItem(create_empty_directory()); });
	connect(remove_files_action, &QAction::triggered, [&]() { remove_item(itm);  });
	connect(rename_action, &QAction::triggered, [&]() { rename_directory(itm);  });
	connect(export_files_action, &QAction::triggered, [&]() { export_files(itm); });
	connect(export_file_action, &QAction::triggered, [&]() { export_files(itm); });

	// Imported File
	if (!itm) {
		menu->addAction(add_directory_action);
	} else if (itm->text(1) == "Directory") {
		menu->addAction(import_files_action);
		menu->addAction(export_files_action);
		menu->addAction(rename_action);
		menu->addAction(remove_files_action);
	} else if (itm->childCount() == 0) {
		menu->addAction(export_file_action);
		menu->addAction(remove_files_action);
	}

	menu->exec(QCursor::pos());
}


QTreeWidgetItem * ImportManager::create_empty_directory() {
	std::string name = "Untitled Directory" + std::to_string(map.imports.directories.size());

	for (size_t i = 0; i < map.imports.directories.size(); i++) {
		std::string s = "Untitled Directory" + std::to_string(i);
		if (map.imports.directories.find(s) == map.imports.directories.end()) {
			name.swap(s);
			break;
		}
	}

	QTreeWidgetItem * itm = create_directory(QString::fromStdString(name));
	map.imports.directories.emplace(name, std::vector<std::string>());
	return itm;
}

void ImportManager::import_files(QTreeWidgetItem* item) {
	QStringList files = QFileDialog::getOpenFileNames(this, "Select one or more files to import",
		".",
		"*.blp *.mdx *.mdl *.tga *.wav *.mp3 *.txt");

	for (auto&& f : files) {
		fs::path s = fs::path(f.toStdString()).stem();

		auto cond = [&s](const Import &imp) { return fs::path(imp.path).stem() == s; };
		auto position = std::find_if(map.imports.imports.begin(), map.imports.imports.end(), cond);

		if (position == map.imports.imports.end()) {
			fs::path file = f.toStdString();
			auto size = fs::file_size(file);

			QString file_name = QString::fromStdString(file.filename().string());
			QString file_type = get_file_type(file);
			QString full_path = generate_full_path(file_name);

			add_child_item(item, file_name, file_type, size, full_path);

			map.imports.imports.emplace_back(true, full_path.toStdString(), f.toStdString(), size);
			map.imports.directories.at(item->text(0).toStdString()).push_back(full_path.toStdString());

			map.imports.import_file(file, full_path.toStdString());
		}
	}
	item->setExpanded(true);
}

void ImportManager::remove_item(QTreeWidgetItem *itm) {
	const int choice = QMessageBox::warning(this, "HiveWE",
					"Are you sure you want to remove " + itm->text(0),
					QMessageBox::Yes | QMessageBox::No);

	if (choice != QMessageBox::Yes) {
		return;
	}

	std::string itm_name = itm->text(0).toStdString();
	if (itm->text(1) == "Directory") {
		auto temp_vec = map.imports.directories[itm_name];
		for (auto&& f : temp_vec) {
			auto cond = [&f](const Import &imp) { return (imp.path == f); };
			auto position = std::remove_if(map.imports.imports.begin(), map.imports.imports.end(), cond);
			if (position != map.imports.imports.end() ) {
				map.imports.remove_import(f);
				map.imports.imports.erase(position);
			}
		}
		map.imports.directories.erase(itm_name);
	} else {
		std::string parent_name = itm->parent()->text(0).toStdString();
		itm_name = itm->text(3).toStdString();

		auto cond = [&itm_name](const Import &imp) { return imp.path == itm_name;  };
		map.imports.imports.erase(std::remove_if(map.imports.imports.begin(), map.imports.imports.end(), cond), map.imports.imports.end());
		auto position = std::find(map.imports.directories.at(parent_name).begin(), map.imports.directories.at(parent_name).end(), itm_name);
		if (position != map.imports.directories.at(parent_name).end()) {
			map.imports.remove_import(itm_name);
			map.imports.directories.at(parent_name).erase(position);
		}
	}

	delete itm;
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

QString ImportManager::get_file_type(fs::path path) {
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
void ImportManager::rename_directory(QTreeWidgetItem * itm) {
	QDialog * diag = new QDialog(this);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok
		| QDialogButtonBox::Cancel);

	connect(buttons, &QDialogButtonBox::rejected, diag, &QDialog::reject);

	QLineEdit * input = new QLineEdit(diag);
	input->setPlaceholderText("Directory name...");
	input->setMaxLength(32);
	input->setText(itm->text(0));

	connect(buttons, &QDialogButtonBox::accepted, [&]() {
		std::string name = input->text().toStdString();
		std::vector <std::string> temp(map.imports.directories.at(itm->text(0).toStdString()));
		map.imports.directories.erase(itm->text(0).toStdString());
		map.imports.directories.emplace(name, temp);

		itm->setText(0, input->text());
		diag->close();
	});
	connect(input, &QLineEdit::textChanged, [&](const QString &text) {
		buttons->button(QDialogButtonBox::Ok)->setDisabled(text.isEmpty() || map.imports.directories.count(text.toStdString()) == 1);
	});

	mainLayout->addWidget(input);
	mainLayout->addWidget(buttons);

	diag->setLayout(mainLayout);
	diag->setWindowTitle("Rename Directory");
	diag->exec();
}


void ImportManager::load_files(std::map<std::string, std::vector<std::string>>& directories, std::vector<Import>& imports) {
	if (directories.empty() && imports.empty()) { 
		return; 
	}

	if (directories.empty()) {
		QTreeWidgetItem * parent = create_empty_directory();
		directories.emplace(parent->text(0).toStdString(), std::vector<std::string>());
		ui->treeWidget->addTopLevelItem(parent);
		for (auto&& imp : imports) {
			directories.at(parent->text(0).toStdString()).push_back(imp.path);

			QString file_name = QString::fromStdString(fs::path(imp.path).filename().string());
			QString file_type = get_file_type(imp.path);
			QString full_path = generate_full_path(file_name);

			add_child_item(parent, file_name, file_type, imp.size, full_path);
		}
	} else {
		std::vector<std::string> temp_vec;

		QTreeWidgetItem * parent;
		for (auto&&[name, files] : directories) {
			std::string s = fs::path(name).stem().string();
			parent = create_directory(QString::fromStdString(s));
			ui->treeWidget->addTopLevelItem(parent);
			for (auto&& f : files) {
				auto cond = [&f](const Import &imp) { return f == imp.path; };
				auto position = std::find_if(imports.begin(), imports.end(), cond);
				if (position == imports.end()) {
					directories.at(name).erase(std::remove(files.begin(), files.end(), f));
				} else {
					temp_vec.push_back(f);

					int idx = std::distance(imports.begin(), position);

					QString file_name = QString::fromStdString(fs::path(f).filename().string());
					QString file_type = get_file_type(f);
					QString file_path = generate_full_path(file_name);

					add_child_item(parent, file_name, file_type, imports.at(idx).size, file_path);
				}
			}

		}

		parent = create_empty_directory();
		ui->treeWidget->addTopLevelItem(parent);

		for (auto && imp : imports) {
			for (auto &&[name, files] : directories) {
				if (files.empty()) {
					continue;
				}

				auto position = std::find(files.begin(), files.end(), imp.path);
				auto vec_pos = std::find(temp_vec.begin(), temp_vec.end(), imp.path);

				if (position == files.end() && vec_pos == temp_vec.end()) {
					temp_vec.push_back(imp.path);
					QString file_name = QString::fromStdString(fs::path(imp.path).filename().string());
					QString file_type = get_file_type(imp.path);
					QString file_path = QString::fromStdString(imp.path);

					add_child_item(parent, file_name, file_type, map.imports.import_size(imp.path), file_path);

					directories.at(parent->text(0).toStdString()).push_back(imp.path);
				}
			}
		}
	}
}

void ImportManager::export_files(QTreeWidgetItem * itm) {
	QFileDialog * fdiag = new QFileDialog(this, "Export Files", ".");
	fdiag->setAcceptMode(QFileDialog::AcceptSave);
	fdiag->setFileMode(QFileDialog::Directory);
	fdiag->setOptions(QFileDialog::ShowDirsOnly);
	std::string path = fdiag->getExistingDirectory(this,"Select Folder",".").toStdString();
	if (itm->childCount() == 0) {
		map.imports.export_file(path, itm->text(3).toStdString());
	} else {
		auto files = map.imports.directories.at(itm->text(0).toStdString());
		for (auto && f : files) {
			map.imports.export_file(path, f);
		}
	}
}


bool ImportManager::eventFilter(QObject*, QEvent *event) {
	if (event->type() == QKeyEvent::KeyPress) {
		QKeyEvent * k_event = dynamic_cast<QKeyEvent *>(event);
		if (k_event->key() == Qt::Key_Delete) {
			remove_item(ui->treeWidget->currentItem());
			return true;
		}
	} 
	return false;
}