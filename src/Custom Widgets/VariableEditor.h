#include <qwidget.h>

#include "Triggers.h"
#include "ui_VariableEditor.h"

class VariableEditor : public QWidget {
	Q_OBJECT
public:
	Ui::VariableEditor ui;
	VariableEditor(TriggerVariable& variable);
};