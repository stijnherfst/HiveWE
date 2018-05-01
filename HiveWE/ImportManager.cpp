#include "stdafx.h"
#include "ui_ImportManager.h"


ImportManager::ImportManager(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::ImportManager)
{
    ui->setupUi(this);

    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeWidget->header()->sortIndicatorOrder();
	ui->treeWidget->header()->resizeSection(0, 220);

    connect(ui->treeWidget,&QTreeWidget::customContextMenuRequested,this,&ImportManager::CustomMenuPopup);

	this->LoadFiles(map.allImports.dirEntries); // Loads pre-existing files if any.
	
	show();
}


ImportManager::~ImportManager()
{
    delete ui;
}

QTreeWidgetItem * ImportManager::CreateDir(QString name) {
    QTreeWidgetItem * itm = new QTreeWidgetItem();
    itm->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    itm->setText(0, name);
	itm->setText(1, "Directory");

	return itm;
}

void ImportManager::AddChildItem(QTreeWidgetItem * itm,QString name,QString itmType,QString itmSize,QString fullPath){
    QTreeWidgetItem * childitm = new QTreeWidgetItem();
    childitm->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
    childitm->setText(0, name);
    childitm->setText(1, itmType);
    childitm->setData(2,Qt::DisplayRole, itmSize);
    childitm->setText(3, fullPath);
    itm->addChild(childitm);
}

void ImportManager::CustomMenuPopup(const QPoint & pos) {
    QTreeWidgetItem * itm = ui->treeWidget->itemAt(pos);
    int column = ui->treeWidget->currentIndex().column();
	int row = ui->treeWidget->currentIndex().row();

    QMenu * menu = new QMenu(this);
    QAction * b = new QAction("Import Files");
    QAction * c = new QAction("Remove File");
    QAction * d = new QAction("Export File");
    QAction * e = new QAction("Export Files");
    QAction * f = new QAction("Add Directory");
    QAction * g = new QAction("Rename");

    connect(b, &QAction::triggered,[&](){ ImportFiles(itm,row); });
    connect(f, &QAction::triggered,[&](){ CreateEmptyDir(); });
    connect(c, &QAction::triggered,[&](){ RemoveItem(itm,row);  });
	connect(g, &QAction::triggered,[&]() { RenameDir(itm,row);  });
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


void ImportManager::CreateEmptyDir(){
	std::string name = "Untitled Directory" + std::to_string(map.allImports.dirEntries.size());
    QTreeWidgetItem * itm = CreateDir(QString::fromStdString(name));
	int isCustom = true;
	map.allImports.dirEntries.push_back(name);
	ui->treeWidget->addTopLevelItem(itm);
}

void ImportManager::ImportFiles(QTreeWidgetItem * itm,int row) {
    QFileDialog * dial = new QFileDialog(this,tr("Import Files"),".");
    dial->setNameFilter(("*.blp *.mdx *.mdl *.tga .wav .mp3 .txt"));
    dial->setFileMode(QFileDialog::ExistingFiles);
    if ( dial->exec() == QFileDialog::Accepted ) {
		if (map.allImports.dirEntries.at(row) != itm->text(0).toStdString() + ".dir" ) 
		{
			map.allImports.dirEntries.push_back(itm->text(0).toStdString());
		}

		for(auto&& f : dial->selectedFiles() ) {
            if ( !importedFiles.contains(f) ) {
            QFileInfo fi(f);
			QString full_path = GenerateFullPath(fi.fileName());
            AddChildItem(itm, fi.baseName(), fi.suffix(),QString::number(fi.size()),  full_path );
			
			int isCustom = true;
			std::string path = full_path.toStdString();
			map.allImports.imports.emplace_back(isCustom,path);
			map.allImports.dirEntries.push_back(path);

			importedFiles.append(fi.baseName());
			}
		
        }
		
        itm->setExpanded(true);
    }
}

void ImportManager::RemoveItem(QTreeWidgetItem *itm,int row) {
	std::string itmName = itm->text(0).toStdString();

	if ( itm->text(1) == "Directory" ) {
		int index = std::find(map.allImports.dirEntries.begin(), map.allImports.dirEntries.end(), itmName) - map.allImports.dirEntries.begin();
		map.allImports.dirEntries.erase(map.allImports.dirEntries.begin() + index);
		// Calculating offset for the deleted directory.
		for (int i = index ; i < map.allImports.dirEntries.size(); i++ ) {
			std::string name = map.allImports.dirEntries[i];
			if (name.substr(name.size() - 4, 4) == ".dir" ) {
				break;
			}
			map.allImports.dirEntries.erase(map.allImports.dirEntries.begin() + i);
		}
	}
	else {
		auto cond = [&itmName](const Import &imp) { return (imp.path == itmName); };
		map.allImports.imports.erase(std::remove_if(map.allImports.imports.begin(), map.allImports.imports.end(), cond), map.allImports.imports.end());
	}

	delete itm;
}

QString ImportManager::GenerateFullPath(QString fileName){
	QString s;
    if ( fileName.startsWith("BTN") )
        { s = "ReplaceableTextures\\CommandButtons\\"; }
    else if ( fileName.startsWith("PAS") )
        { s = "ReplaceableTextures\\PassiveButtons\\"; }
    else if ( fileName.startsWith("DISBTN") || fileName.startsWith("DISPAS") )
        { s = "ReplaceableTextures\\CommandButtonsDisabled\\"; }
    return QString(s + fileName);
}

void ImportManager::RenameDir(QTreeWidgetItem * itm,int row) {
	QDialog * diag = new QDialog(this);
	QGridLayout * gLayout = new QGridLayout();
	QVBoxLayout * vLayout = new QVBoxLayout();
	QHBoxLayout * hLayout = new QHBoxLayout();

	QLineEdit * input = new QLineEdit(diag);
	input->setPlaceholderText("Directory name...");
	input->setMaxLength(32);
	input->setText(itm->text(0));

	vLayout->addWidget(input);

	QPushButton * renameBut = new QPushButton();
	renameBut->setText("Rename");
	connect(renameBut, &QPushButton::clicked, [&]() { 
		std::string name = input->text().toStdString();
		int index = std::find_if(map.allImports.dirEntries.begin(), map.allImports.dirEntries.end(), name) - map.allImports.dirEntries.begin();

		map.allImports.dirEntries.erase(map.allImports.dirEntries.begin() + index);
		map.allImports.dirEntries.insert(map.allImports.dirEntries.begin() + index, name);

		itm->setText(0, input->text() );
		diag->close();
	});
	connect(input, &QLineEdit::textChanged, [&](QString text) { 
		if ( text.isEmpty() || std::find_if(map.allImports.dirEntries.begin(), map.allImports.dirEntries.end(), input->text().toStdString()) != map.allImports.dirEntries.end()  ) {
			renameBut->setEnabled(false);
		}
		else {
			renameBut->setEnabled(true);
		}
	});
	hLayout->addWidget(renameBut);

	QPushButton * cancelBut = new QPushButton();
	cancelBut->setText("Cancel");
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


void ImportManager::LoadFiles(std::vector<std::string> &dirEntries) {
	if (dirEntries.empty() && map.allImports.imports.empty()) { return; }
	
	int count = dirEntries.size();

	for (auto&& name : dirEntries) {

		QTreeWidgetItem * parent = CreateDir(QString::fromStdString(name));
		
		ui->treeWidget->addTopLevelItem(parent);

		for (int i = 0;  i < count; i++) {
			Import imp = map.allImports.imports[i];

			std::string name = (split(imp.path, '\\').size() == 0) ? imp.path : split(imp.path,'\\').back();
			std::string fileType = imp.path.substr(imp.path.size() - 3, 3);
			AddChildItem(parent, QString::fromStdString(name.substr(0,name.size() - 4)), "", QString::fromStdString(fileType) , QString::fromStdString(imp.path));
		}
	}
}