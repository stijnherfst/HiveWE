#pragma once

#include "ui_ImportManager.h"

class ImportManager : public QMainWindow {
	Q_OBJECT

	QTreeWidgetItem* create_directory(QString name);
	QTreeWidgetItem* create_empty_directory();
	void rename_directory(QTreeWidgetItem * itm);

	void add_child_item(QTreeWidgetItem * itm, QString name, QString item_type, int item_size, QString full_path) const;
	void custom_menu_popup(const QPoint & pos);
	void import_files(QTreeWidgetItem * itm);
	void export_files(QTreeWidgetItem * itm);
	void remove_item(QTreeWidgetItem * itm);

	void load_files(std::map<std::string, std::vector<std::string>> &directories, std::vector<Import> &imports);

	static QString generate_full_path(QString file_name);

	static QString get_file_type(fs::path path);

protected:
	bool eventFilter(QObject* obj, QEvent *event) override;

public:
	explicit ImportManager(QWidget *parent = nullptr);
	~ImportManager();

private:
	Ui::ImportManager *ui;
};