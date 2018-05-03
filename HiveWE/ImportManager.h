#pragma once

namespace Ui {
	class ImportManager;
}

class ImportManager : public QMainWindow
{
	Q_OBJECT

	QTreeWidgetItem * CreateDir(QString name);

	void AddChildItem(QTreeWidgetItem * itm, QString name, QString itmType, QString itmSize, QString fullPath);
	void CustomMenuPopup(const QPoint & pos);
	void RenameDir(QTreeWidgetItem * itm);
	void ImportFiles(QTreeWidgetItem * itm);
	void ExportFiles();
	QTreeWidgetItem * CreateEmptyDir();
	void RemoveItem(QTreeWidgetItem * itm);

	void LoadFiles(std::map<std::string,std::vector<std::string>> &dirEntries,std::vector<Import> &imports);

	QString GenerateFullPath(QString fileName);
public:
	explicit ImportManager(QWidget *parent = 0);
	~ImportManager();

private:
	Ui::ImportManager *ui;
};
