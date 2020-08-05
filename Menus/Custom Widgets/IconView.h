#pragma once

#include <unordered_map>

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QToolButton>

#include <absl/container/flat_hash_map.h>

class IconButton : public QToolButton {
	Q_OBJECT

  public:
	QSize minimumSizeHint() const override {
		return QWidget::minimumSizeHint();
	}

	QSize sizeHint() const override {
		return QWidget::sizeHint();
	}
};

class IconView : public QWidget {
	Q_OBJECT

	QComboBox* type = new QComboBox;
	QLineEdit* search = new QLineEdit;

	//absl::flat_hash_map<std::string, std::string> icons;
	std::unordered_map<std::string, QString> icons;
	
public:
	IconView(QWidget* parent = nullptr);
};