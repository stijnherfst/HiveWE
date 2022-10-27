#include "ModelEditor.h"

#include <QTableView>
#include <QLineEdit>
#include <QToolBar>
#include <QDialogButtonBox>
#include <QSortFilterProxyModel>
#include <QPushButton>
#include <QTimer>

#include "SingleModel.h"
#include "UnitSelector.h"

ModelEditor::ModelEditor(QWidget* parent) : QMainWindow(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	show();
}