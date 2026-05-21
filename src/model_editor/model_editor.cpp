#include "model_editor.h"

#include "model_editor_glwidget.h"

#include <QObject>
#include <QLabel>
#include <QVBoxLayout>
#include <QToolButton>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QSizePolicy>

#include <DockAreaTitleBar.h>
#include <DockAreaTabBar.h>
#include <DockComponentsFactory.h>

#include "model_view.h"

import std;
import BinaryReader;
import Hierarchy;
import MDX;
import Utilities;

class CCustomComponentsFactory: public ads::CDockComponentsFactory {
  public:
	using Super = ads::CDockComponentsFactory;

	explicit CCustomComponentsFactory(ModelEditor& model_editor) : Super(), model_editor(model_editor) {}

	ModelEditor& model_editor;

	ads::CDockAreaTitleBar* createDockAreaTitleBar(ads::CDockAreaWidget* dock_area) const override {
		const auto title_bar = new ads::CDockAreaTitleBar(dock_area);

		const auto add_tab = new QToolButton(title_bar);
		add_tab->setToolTip("Open Model");
		add_tab->setIcon(QIcon("data/icons/model_editor/plus.png"));
		add_tab->setMinimumSize(23, 23);
		add_tab->setIconSize(QSize(13, 13));
		add_tab->setAutoRaise(true);

		const int index = title_bar->indexOf(title_bar->tabBar());
		title_bar->insertWidget(index + 1, add_tab);

		QObject::connect(add_tab, &QToolButton::clicked, [=] {
			model_editor.browse_models(dock_area);
		});

		return title_bar;
	}
};

ModelEditor::ModelEditor(QWidget* parent) : QMainWindow(parent) {
	// setWindowTitle("Model Editor");
	setWindowTitle("");
	setAttribute(Qt::WA_DeleteOnClose);
	resize(1200, 800);

	setWindowFlag(Qt::ExpandedClientAreaHint, true);
	setWindowFlag(Qt::NoTitleBarBackgroundHint, true);
	setAttribute(Qt::WA_LayoutOnEntireRect, true);

	dock_manager = new ads::CDockManager;
	dock_manager->setStyleSheet("");
	dock_manager->setConfigFlag(ads::CDockManager::DockAreaHasCloseButton, false);
	dock_manager->setConfigFlag(ads::CDockManager::DockAreaHasTabsMenuButton, false);
	dock_manager->setConfigFlag(ads::CDockManager::DockAreaHasUndockButton, false);
	dock_manager->setConfigFlag(ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility, false);
	dock_manager->setConfigFlag(ads::CDockManager::AlwaysShowTabs, true);

	dock_manager->setComponentsFactory(new CCustomComponentsFactory(*this));
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

void ModelEditor::open_file() {
	const QSettings settings;

	const QString file_name = QFileDialog::getOpenFileName(
		this,
		"Open Model File",
		settings.value("openDirectory", QDir::current().path()).toString(),
		"MDLX (*.mdl *.MDL *.mdx *.MDX)"
	);

	if (file_name == "") {
		return;
	}

	const fs::path path = file_name.toStdString();

	const auto result = this->open_model(path, true);
	if (!result.has_value()) {
		QMessageBox::information(this, "Opening model failed", result.error().c_str());
	}
	dock_manager->addDockWidget(ads::CenterDockWidgetArea, result.value(), dock_area);
}

void ModelEditor::browse_models(ads::CDockAreaWidget* parent) {
	QDialog* dialog = new QDialog(parent, Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->resize(1280, 720);
	dialog->setWindowModality(Qt::WindowModality::WindowModal);

	ModelView* view = new ModelView();

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
	connect(view, &ModelView::doubleClicked, dialog, &QDialog::accept);

	connect(dialog, &QDialog::accepted, [=] {
		const auto result = this->open_model(view->current_model_path().toStdString(), false);
		if (!result.has_value()) {
			QMessageBox::information(this, "Opening model failed", result.error().c_str());
		}
		dock_manager->addDockWidget(ads::CenterDockWidgetArea, result.value(), parent);
	});

	QVBoxLayout* layout = new QVBoxLayout(dialog);
	layout->addWidget(view);
	layout->addWidget(buttonBox);

	dialog->show();
}

std::expected<ads::CDockWidget*, std::string> ModelEditor::open_model(const fs::path& path, const bool local_file) const {
	std::shared_ptr<mdx::MDX> mdx;
	if (path.extension() == ".mdl" || path.extension() == ".MDL") {
		const auto file = read_text_file(path);
		auto result = mdx::MDX::from_mdl(file);
		if (!result.has_value()) {
			return std::unexpected(result.error());
		}
		mdx = std::make_shared<mdx::MDX>(std::move(result.value()));
	} else if (path.extension() == ".mdx" || path.extension() == ".MDX") {
		auto result = [&] {
			if (local_file) {
				return read_file(path);
			} else {
				return hierarchy.open_file(path);
			}
		}();

		if (!result.has_value()) {
			return std::unexpected(result.error());
		}
		mdx = std::make_shared<mdx::MDX>(result.value());
	} else {
		return std::unexpected("Unsupported file type");
	}

	auto* gl_widget = new ModelEditorGLWidget(nullptr, mdx);

	auto* dock_tab = dock_manager->createDockWidget("");
	dock_tab->setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetDeleteOnClose, true);
	dock_tab->setWidget(gl_widget);
	dock_tab->setWindowTitle(QString::fromStdString(path.filename().string()));

	return dock_tab;
}
