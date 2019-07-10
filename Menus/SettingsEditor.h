#pragma once

#include "ui_SettingsEditor.h"

class SettingsEditor : public QDialog {
	Q_OBJECT

public:
	explicit SettingsEditor(QWidget* parent = nullptr);
private:
	Ui::SettingsEditor ui;
	void save() const;
};