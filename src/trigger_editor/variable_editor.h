#include <qwidget.h>

#include "ui_variable_editor.h"

import Triggers;

class VariableEditor : public QWidget {
	Q_OBJECT
public:
	Ui::VariableEditor ui;
	VariableEditor(TriggerVariable& variable);
};