#pragma once

#include <unordered_map>
#include <memory>

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

class ModelView : public QWidget {
	Q_OBJECT

	QComboBox* type = new QComboBox;
	QLineEdit* search = new QLineEdit;
	QLineEdit* finalPath = new QLineEdit;
	QPushButton* open_in_model_editor = new QPushButton;

public:
	ModelView(QWidget* parent = nullptr);

	QString current_model_path();
	void set_current_model_path(const QString& path);
};