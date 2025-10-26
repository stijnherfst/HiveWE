#include "model_editor.h"

#include "model_editor_glwidget.h"

#include <QLabel>

import std;
import BinaryReader;
import Hierarchy;
import MDX;
import Utilities;

ModelEditor::ModelEditor(QWidget* parent) : QMainWindow(parent) {
	setAttribute(Qt::WA_DeleteOnClose);
	resize(1200, 800);

	dock_manager = new ads::CDockManager;
	dock_manager->setStyleSheet("");
	setCentralWidget(dock_manager);

	QLabel* image = new QLabel();
	image->setPixmap(QPixmap("data/icons/model_editor/background.png"));
	image->setAlignment(Qt::AlignCenter);

	const auto centraldock_widget = dock_manager->createDockWidget("CentralWidget");
	centraldock_widget->setWidget(image);
	centraldock_widget->setFeature(ads::CDockWidget::NoTab, true);
	dock_area = dock_manager->setCentralWidget(centraldock_widget);

	show();
}

std::expected<void, std::string> ModelEditor::open_model(const fs::path& path, const bool local_file) const {
	auto reader = [&] {
		if (local_file) {
			return read_file(path);
		} else {
			return hierarchy.open_file(path);
		}
	 }();

	if (!reader) {
		return std::unexpected(reader.error());
	}

	const auto mdx = std::make_shared<mdx::MDX>(reader.value());
	auto* gl_widget = new ModelEditorGLWidget(nullptr, mdx);

	auto* dock_tab = dock_manager->createDockWidget("");
	dock_tab->setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetDeleteOnClose, true);
	dock_tab->setWidget(gl_widget);
	// dock_tab->setObjectName(QString::fromStdString(item->id));
	dock_tab->setWindowTitle(QString::fromStdString(path.filename().string()));

	dock_manager->addDockWidget(ads::CenterDockWidgetArea, dock_tab, dock_area);
	return {};
}
