#ifndef IMPORTMANAGER_H
#define IMPORTMANAGER_H

#pragma once

namespace Ui {
class ImportManager;
}

class ImportManager : public QMainWindow
{
    Q_OBJECT

    QList<QString> importedFiles;
	QTreeWidgetItem * CreateDir(QString name);

    void AddChildItem(QTreeWidgetItem * itm,QString name,QString itmType,QString itmSize,QString fullPath);
    void CustomMenuPopup(const QPoint & pos);
    void RenameDir(QTreeWidgetItem * itm,int row);
    void ImportFiles(QTreeWidgetItem * itm, int row);
    void ExportFiles();
    void CreateEmptyDir();
    void RemoveItem(QTreeWidgetItem * itm,int row);

	void LoadFiles(std::vector<std::string> &dirEntries);

    QString GenerateFullPath(QString fileName);
public:
    explicit ImportManager(QWidget *parent = 0);
    ~ImportManager();

private:
    Ui::ImportManager *ui;
};

#endif // IMPORTMANAGER_H
