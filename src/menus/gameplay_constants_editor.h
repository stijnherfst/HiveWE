#pragma once

#include <QDialog>
#include <QLineEdit>

class GameplayConstantsEditor : public QDialog {
	Q_OBJECT
public:
	explicit GameplayConstantsEditor(QWidget* parent = nullptr);

private:
	QLineEdit* search;
};
