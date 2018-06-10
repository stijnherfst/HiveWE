#include "stdafx.h"
#include "HiveWE.h"
#include <QtWidgets/QApplication>

struct TT {
	glm::vec3 position;

	TT(glm::vec3 p) : position(p) {}
};

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);

	QCoreApplication::setOrganizationName("HiveWE");
	QCoreApplication::setApplicationName("HiveWE");

	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(8);
	format.setVersion(4, 5);
	format.setProfile(QSurfaceFormat::CoreProfile);
	QSurfaceFormat::setDefaultFormat(format);

	HiveWE w;
	w.showMaximized();
	return QApplication::exec();
}