#include "stdafx.h"
#include "ui_ImportManager.h"


ImportManager::ImportManager(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::ImportManager)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->treeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

	ui->treeWidget->setFocusPolicy(Qt::ClickFocus);
	ui->treeWidget->setFocus();
	ui->treeWidget->installEventFilter(this);
	ui->treeWidget->setUniformRowHeights(true);
	ui->treeWidget->header()->resizeSection(0, 220);
	connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested, this, &ImportManager::CustomMenuPopup);


	LoadFiles(map.imports.directories, map.imports.imports); // Loads pre-existing files if any.

	show();
}


ImportManager::~ImportManager()
{
	delete ui;
}

QTreeWidgetItem * ImportManager::CreateDir(QString name) {
	QTreeWidgetItem * itm = new QTreeWidgetItem();
	itm->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
	itm->setText(0, name);
	itm->setText(1, "Directory");

	return itm;
}

void ImportManager::AddChildItem(QTreeWidgetItem * itm, QString name, QString itmType, QString itmSize, QString fullPath) {
	QTreeWidgetItem * childitm = new QTreeWidgetItem();
	childitm->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
	childitm->setText(0, name);
	childitm->setText(1, itmType);
	childitm->setData(2, Qt::DisplayRole, itmSize);
	childitm->setText(3, fullPath);

	itm->addChild(childitm);
}

void ImportManager::CustomMenuPopup(const QPoint & pos) {
	QTreeWidgetItem * itm = ui->treeWidget->itemAt(pos);

	QMenu * menu = new QMenu(this);
	QAction * b = new QAction("Import Files");
	QAction * c = new QAction("Remove File");
	QAction * d = new QAction("Export File");
	QAction * e = new QAction("Export Files");
	QAction * f = new QAction("Add Directory");
	QAction * g = new QAction("Rename");

	connect(b, &QAction::triggered, [&]() { ImportFiles(itm); });
	connect(f, &QAction::triggered, [&]() { ui->treeWidget->addTopLevelItem(CreateEmptyDir()); });
	connect(c, &QAction::triggered, [&]() { RemoveItem(itm);  });
	connect(g, &QAction::triggered, [&]() { RenameDir(itm);  });
	connect(e, &QAction::triggered, [&]() { ExportFiles(itm); });
	connect(d, &QAction::triggered, [&]() { ExportFiles(itm); });

	// Imported File
	if (!itm) {
		menu->addAction(f);
	}
	else if (itm && itm->text(1) == "Directory") {
		menu->addAction(b);
		menu->addSeparator();
		menu->addAction(e);
		menu->addSeparator();
		menu->addAction(g);
		menu->addSeparator();
		menu->addAction(c);
	}
	else if (itm && itm->childCount() == 0)
	{
		menu->addAction(d);
		menu->addSeparator();
		menu->addAction(c);
	}


	menu->exec(QCursor::pos());
}


QTreeWidgetItem * ImportManager::CreateEmptyDir() {
	std::string name = "Untitled Directory" + std::to_string(map.imports.directories.size());

	// Maybe an error message box  ?
	for (size_t i = 0; i < map.imports.directories.size(); i++) {
		std::string s = "Untitled Directory" + std::to_string(i);
		if (map.imports.directories.find(s + ".dir") == map.imports.directories.end()) {
			name.swap(s);
			break;
		}
	}

	QTreeWidgetItem * itm = CreateDir(QString::fromStdString(name));
	map.imports.directories.emplace(name + ".dir", std::vector<std::string>());
	return itm;
}

void ImportManager::ImportFiles(QTreeWidgetItem * itm) {
	QFileDialog * dial = new QFileDialog(this, tr("Import Files"), ".");
	dial->setNameFilter(("*.blp *.mdx *.mdl *.tga *.wav *.mp3 *.txt"));
	dial->setFileMode(QFileDialog::ExistingFiles);
	if (dial->exec() == QFileDialog::Accepted) {
		for (auto&& f : dial->selectedFiles()) {

			std::string s = fs::path(f.toStdString()).stem().string();

			auto cond = [&s](const Import &imp) { return fs::path(imp.path).stem().string() == s; };
			auto position = std::find_if(map.imports.imports.begin(), map.imports.imports.end(), cond);

			if (position == map.imports.imports.end()) {
				QString file_name = QString::fromStdString(fs::path(file).filename().string());
				QString file_type = QString::fromStdString(fs::path(file).extension().string());
				QString file_size = QString::number(fs::file_size(fs::path(file)));
				QString full_path = GenerateFullPath(file_name);

				AddChildItem(itm, file_name, file_type, file_size, full_path);

				int custom = true;

				map.imports.imports.emplace_back(custom, full_path.toStdString(), f.toStdString());
				map.imports.directories.at(itm->text(0).toStdString() + ".dir").push_back(full_path.toStdString());

			}

		}

		itm->setExpanded(true);
	}
}

void ImportManager::RemoveItem(QTreeWidgetItem *itm) {
	std::string itm_name = itm->text(0).toStdString();

	if (!RemoveMessageBox(itm->text(0))) { return; }

	if (itm->text(1) == "Directory") {
		auto temp_vec = map.imports.directories.at(itm_name + ".dir");
		for (auto&& f : temp_vec) {
			auto cond = [&f](const Import &imp) { return (imp.path == f); };
			auto position = std::remove_if(map.imports.imports.begin(), map.imports.imports.end(), cond);
			if (position != map.imports.imports.end() ) {
				map.imports.remove_import(f);
				map.imports.imports.erase(position);
			}
		}
		map.imports.directories.erase(itm_name + ".dir");
	} else {
		std::string parent_name = itm->parent()->text(0).toStdString() + ".dir";
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

QString ImportManager::GenerateFullPath(QString fileName) {
	if (fileName.startsWith("BTN")) {
		return "ReplaceableTextures\\CommandButtons\\" + fileName;
	} else if (fileName.startsWith("PAS")) {
		return "ReplaceableTextures\\PassiveButtons\\" + fileName;
	} else if (fileName.startsWith("DISBTN") || fileName.startsWith("DISPAS")) {
		return "ReplaceableTextures\\CommandButtonsDisabled\\" + fileName;
	}
	return fileName;
}

void ImportManager::RenameDir(QTreeWidgetItem * itm) {
	QDialog * diag = new QDialog(this);
	QGridLayout * gLayout = new QGridLayout();
	QVBoxLayout * vLayout = new QVBoxLayout();
	QHBoxLayout * hLayout = new QHBoxLayout();

	QLineEdit * input = new QLineEdit(diag);
	input->setPlaceholderText("Directory name...");
	input->setMaxLength(32);
	input->setText(itm->text(0));

	vLayout->addWidget(input);

	QPushButton * renameBut = new QPushButton("Rename");

	connect(renameBut, &QPushButton::clicked, [&]() {
		std::string name = input->text().toStdString() + ".dir";
		std::vector <std::string> temp(map.imports.directories.at(itm->text(0).toStdString() + ".dir"));
		map.imports.directories.erase(itm->text(0).toStdString() + ".dir");
		map.imports.directories.emplace(name, temp);

		itm->setText(0, input->text());
		diag->close();
	});
	connect(input, &QLineEdit::textChanged, [&](const QString &text) {
		renameBut->setDisabled(text.isEmpty() || map.imports.directories.count(text.toStdString() + ".dir") == 1);
	});
	hLayout->addWidget(renameBut);

	QPushButton * cancelBut = new QPushButton("Cancel");

	connect(cancelBut, &QPushButton::clicked, [&]() { diag->close(); });
	hLayout->addWidget(cancelBut);

	gLayout->addLayout(vLayout, 0, 0);
	gLayout->addLayout(hLayout, 1, 0);

	diag->setLayout(gLayout);
	diag->setFixedSize(220, 90);
	diag->setSizeGripEnabled(false);
	diag->setWindowTitle("Rename Directory");
	diag->exec();
}


void ImportManager::LoadFiles(std::map<std::string, std::vector<std::string>> &directories, std::vector<Import> &imports) {
	if (directories.empty() && imports.empty()) { return; }
	else if (directories.empty()) {
		QTreeWidgetItem * parent = CreateEmptyDir();
		directories.emplace(parent->text(0).toStdString() + ".dir", std::vector<std::string>());
		ui->treeWidget->addTopLevelItem(parent);
		for (auto&& imp : imports) {
			directories.at(parent->text(0).toStdString() + ".dir").push_back(imp.path);

			QString file_name = QString::fromStdString(fs::path(imp.path).filename().string());
			QString file_type = QString::fromStdString(fs::path(imp.path).extension().string());
			QString full_path = GenerateFullPath(file_name);

			AddChildItem(parent, file_name, file_type, "", full_path);
		}
	}
	else {
		std::vector<std::string> temp_vec;

		QTreeWidgetItem * parent;
		for (auto&&[name, files] : directories) {
			std::string s = fs::path(name).stem().string();
			parent = CreateDir(QString::fromStdString(s));
			ui->treeWidget->addTopLevelItem(parent);
			for (auto&& f : files) {
				auto cond = [&f](const Import &imp) { return f == imp.path; };
				auto position = std::find_if(imports.begin(), imports.end(), cond);
				if (position == imports.end()) {
					directories.at(name).erase(std::remove(files.begin(), files.end(), f));
				}
				else {
					temp_vec.push_back(f);

					QString file_name = QString::fromStdString(fs::path(f).filename().string());
					QString file_type = QString::fromStdString(fs::path(f).extension().string());
					QString file_path = GenerateFullPath(file_name);

					AddChildItem(parent, file_name, file_type, "", file_path);
				}
			}

		}

		parent = CreateEmptyDir();
		ui->treeWidget->addTopLevelItem(parent);

		for (auto && imp : imports) {
			for (auto &&[name, files] : directories) {
				if (files.size() == 0 || imp.path == "war3map.dir") { continue; }
				auto position = std::find(files.begin(), files.end(), imp.path);
				auto vec_pos = std::find(temp_vec.begin(), temp_vec.end(), imp.path);

				if (position == files.end() && vec_pos == temp_vec.end()) {
					temp_vec.push_back(imp.path);
					QString file_name = QString::fromStdString(fs::path(imp.path).filename().string());
					QString file_type = QString::fromStdString(fs::path(imp.path).extension().string());
					QString file_path = QString::fromStdString(imp.path);

					AddChildItem(parent, file_name, file_type, "", file_path);

					directories.at(parent->text(0).toStdString() + ".dir").push_back(imp.path);
				}
			}
		}

		if (parent->childCount() == 0) {
			directories.erase(parent->text(0).toStdString() + ".dir");
			delete parent;
		}

	}

}

void ImportManager::ExportFiles(QTreeWidgetItem * itm) {
	QFileDialog * fdiag = new QFileDialog(this, "Export Files", ".");
	fdiag->setAcceptMode(QFileDialog::AcceptSave);
	fdiag->setFileMode(QFileDialog::Directory);
	fdiag->setOptions(QFileDialog::ShowDirsOnly);
	std::string path = fdiag->getExistingDirectory(this,"Select Folder",".").toStdString();
	if (itm->childCount() == 0) {
		auto files = map.imports.directories.at(itm->text(0).toStdString() + ".dir");
		for (auto && f : files) {
			map.imports.export_file(path, f);
		}
	}
	else {
		map.imports.export_file(path, itm->text(3).toStdString());
	}
}


bool ImportManager::eventFilter(QObject *obj, QEvent *event) {
	if (event->type() == QKeyEvent::KeyPress) {
		QKeyEvent * k_event = dynamic_cast<QKeyEvent *>(event);
		if (k_event->key() == Qt::Key_Delete) {
			RemoveItem(ui->treeWidget->currentItem());
			return true;
		}
	} 
	return false;
}


bool ImportManager::RemoveMessageBox(QString msg) {
	QMessageBox m_box;
	m_box.setWindowTitle("Warning");
	m_box.setText("Are you sure you want to remove " + msg + " ?");
	m_box.setStandardButtons(QMessageBox::Yes);
	m_box.addButton(QMessageBox::No);
	return ( m_box.exec() == QMessageBox::Yes );
}
