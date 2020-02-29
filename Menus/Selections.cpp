#include "Selections.h"

Selections::Selections(QWidget* parent) : QDialog(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	show();


}

Selections::~Selections() {

}