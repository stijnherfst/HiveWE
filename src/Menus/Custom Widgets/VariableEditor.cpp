#include "VariableEditor.h"

VariableEditor::VariableEditor(TriggerVariable& variable) : QWidget() {
	ui.setupUi(this);
	ui.name->setText(QString::fromStdString(variable.name));
	ui.type->setText(QString::fromStdString(variable.type));
	ui.array->setChecked(variable.is_array);
	ui.array_size->setEnabled(variable.is_array);
	ui.array_size->setValue(variable.array_size);
	ui.value->setText(QString::fromStdString(variable.initial_value));
	connect(ui.array, &QCheckBox::stateChanged, ui.array_size, &QSpinBox::setEnabled);
}