#include "stdafx.h"

#ifdef WIN32
// To force HiveWE to run on the discrete GPU if available
extern "C" {
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
}
#endif

int main(int argc, char *argv[]) {
	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(8);
	format.setVersion(4, 5);
	format.setProfile(QSurfaceFormat::CoreProfile);
	QSurfaceFormat::setDefaultFormat(format);

	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	QCoreApplication::setOrganizationName("HiveWE");
	QCoreApplication::setApplicationName("HiveWE");

	QApplication a(argc, argv);

	HiveWE w;
	w.showMaximized();
	return QApplication::exec();
}