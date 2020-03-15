#include "ObjectEditor.h"

#include <QSortFilterProxyModel>


#include "UnitTreeModel.h"

void ObjectEditor::item_clicked(const QModelIndex& index) {
	UnitTreeItem* item = static_cast<UnitTreeItem*>(index.internalPointer());
	if (item->tableRow > 0) {
		ads::CDockWidget* dock_tab = new ads::CDockWidget("");
		dock_tab->setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetDeleteOnClose, true);

		connect(dock_tab, &ads::CDockWidget::closeRequested, [&, dock_tab]() {
			if (dock_area->dockWidgets().contains(dock_tab) && dock_area->dockWidgetsCount() == 1) {
				dock_area = nullptr;
			}
		});

		std::string id = units_slk.data("unitid", item->tableRow);

		UnitSingleModel* single_model = new UnitSingleModel(this);
		single_model->setSourceModel(model);
		single_model->setUnitID(id);

		//connect(single_model, &UnitSingleModel::dataChanged, [](const QModelIndex &topLeft, const QModelIndex& bottomRight) {
		//	
		//});

		QTableView* view = new QTableView;
		view->horizontalHeader()->hide();
		view->setAlternatingRowColors(true);
		view->setVerticalHeader(new AlterHeader(Qt::Vertical, view));
		view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
		view->setIconSize({ 24, 24 });
		view->setModel(single_model);
		dock_tab->setWidget(view);
		dock_tab->setWindowTitle(QString::fromStdString(units_slk.data("name", item->tableRow)));
		dock_tab->setIcon(explorerModel->data(index, Qt::DecorationRole).value<QIcon>());

		if (dock_area == nullptr) {
			dock_area = dock_manager->addDockWidget(ads::RightDockWidgetArea, dock_tab, dock_area);
		} else {
			dock_manager->addDockWidget(ads::CenterDockWidgetArea, dock_tab, dock_area);
		}
	}
}

ObjectEditor::ObjectEditor(QWidget* parent) : QMainWindow(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	dock_manager->setConfigFlag(ads::CDockManager::eConfigFlag::AllTabsHaveCloseButton);
	dock_manager->setConfigFlag(ads::CDockManager::eConfigFlag::DockAreaDynamicTabsMenuButtonVisibility);

	model = new UnitModel;

	explorerModel = new UnitTreeModel;
	explorerModel->setSourceModel(model);
	explorer->setModel(explorerModel);
	explorer->header()->hide();

	setCentralWidget(dock_manager);

	ads::CDockWidget* explorer_widget = new ads::CDockWidget("Unit Explorer");
	explorer_widget->setObjectName("-1");
	explorer_widget->setFeature(ads::CDockWidget::DockWidgetClosable, false);
	explorer_widget->setWidget(explorer);
	dock_manager->addDockWidget(ads::LeftDockWidgetArea, explorer_widget);
	dock_manager->setStyleSheet("");

	connect(explorer, &QTreeView::doubleClicked, this, &ObjectEditor::item_clicked);

	show();
}