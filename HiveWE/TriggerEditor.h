#pragma once

#include "ui_TriggerEditor.h"

class TriggerEditor : public QMainWindow {
	Q_OBJECT

public:
	TriggerEditor(QWidget* parent = nullptr);

private:
	Ui::TriggerEditor ui;
};