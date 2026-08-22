#pragma once

#include <QWidget>
#include <string>

class QComboBox;
class QLineEdit;
class QListView;
class UnitListModel;
class UnitListFilter;

class UnitSelector : public QWidget {
	Q_OBJECT

  public:
	explicit UnitSelector(QWidget* parent = nullptr);

	UnitListModel* list_model;
	UnitListFilter* filter_model;

	QComboBox* race;
	QLineEdit* search;
	QListView* units;

  public slots:
	void forceSelection();

  signals:
	void unitSelected(std::string id);
};
