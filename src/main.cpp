#include <glad/glad.h>
#define QT_NO_OPENGL

#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QFont>
#include <QPalette>
#include <QSurfaceFormat>
#include <QSettings>
#include <QStyleFactory>

//import MDX;
import test;

#include "main_window/hivewe.h"
#include "DockManager.h"

#ifdef WIN32
// To force HiveWE to run on the discrete GPU if available
extern "C" {
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
}
#endif

int main(int argc, char *argv[]) {
//	std::string b = R"(
//Version {
//	FormatVersion 1000,
//}
//Model "Wi sp" {
//	BlendTime 150,
//	MinimumExtent { -134.261, -213.936, -143.006 },
//	MaximumExtent { 154.078, 213.709, 284.64 },
//}
//)";
	
	//mdx::MDX::from_mdl(b);
	//execute_tests();

	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(8);
	format.setVersion(4, 5);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setSwapInterval(1);
	//format.setColorSpace(QSurfaceFormat::sRGBColorSpace);
	QSurfaceFormat::setDefaultFormat(format);

	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	QCoreApplication::setOrganizationName("HiveWE");
	QCoreApplication::setApplicationName("HiveWE");

	QLocale::setDefault(QLocale("en_US"));
	
	// Create a dark palette
	// For some magically unknown reason Qt draws Qt::white text as black, so we use QColor(255, 254, 255) instead
	QPalette darkPalette;
	darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
	darkPalette.setColor(QPalette::WindowText, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
	darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
	darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
	darkPalette.setColor(QPalette::ToolTipBase, QColor(66, 66, 66));
	darkPalette.setColor(QPalette::ToolTipText, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::Text, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::PlaceholderText, Qt::gray);
	darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
	darkPalette.setColor(QPalette::Dark, QColor(35, 35, 35));
	darkPalette.setColor(QPalette::Shadow, QColor(20, 20, 20));
	darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
	darkPalette.setColor(QPalette::ButtonText, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
	darkPalette.setColor(QPalette::BrightText, Qt::red);
	darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
	darkPalette.setColor(QPalette::HighlightedText, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));

	QApplication::setPalette(darkPalette);
	QApplication::setStyle("Fusion");
	
	QApplication a(argc, argv);

	ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting);
	ads::CDockManager::setConfigFlag(ads::CDockManager::AllTabsHaveCloseButton);
	ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility);
	ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize);
	ads::CDockManager::setConfigFlag(ads::CDockManager::MiddleMouseButtonClosesTab);

	QSettings settings;
	QFile file("Data/Themes/" + settings.value("theme", "Dark").toString() + ".qss");
	file.open(QFile::ReadOnly);
	a.setStyleSheet(QLatin1String(file.readAll()));

	HiveWE w;
	return QApplication::exec();
}