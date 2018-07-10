#include "stdafx.h"

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
	
	//HANDLE handle;
	//const bool opened = CascOpenStorage(L"C:\\Program Files (x86)\\Warcraft III Public Test\\Data", CASC_LOCALE_ALL, &handle);
	//if (!opened) {
	//	std::wcout << "Error opening with error:" << GetLastError() << std::endl;
	//}

	HiveWE w;
	w.showMaximized();
	return QApplication::exec();
}