#pragma once

#include "ui_ImportManager.h"

class ImportManager : public QMainWindow {
	Q_OBJECT

	void create_directory(QTreeWidgetItem* parent);
	void rename_directory(QTreeWidgetItem* item);
	void custom_menu_popup(const QPoint& pos);

	void import_files(QTreeWidgetItem* item);
	void export_files(QTreeWidgetItem* item);

	void remove_item(QTreeWidgetItem* item);
	void edit_item(QTreeWidgetItem* item);

	void load_files(const std::vector<ImportItem>& items) const;

	//static QString generate_full_path(QString file_name);
	//static QString get_file_type(const fs::path& path);

	Ui::ImportManager ui;

	QIcon folder_icon;
	QIcon file_icon;

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;

public:
	explicit ImportManager(QWidget* parent = nullptr);

	void items_changed();
};