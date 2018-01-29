#include "stdafx.h"

HiveWE::HiveWE(QWidget *parent) : QMainWindow(parent) {
	fs::path directory = find_warcraft_directory();
	while (!fs::exists(directory / "War3Patch.mpq")) {
		directory = QFileDialog::getExistingDirectory(this, "Select Warcraft Directory", "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdWString();
	}
	hierarchy.warcraft_directory = directory;

	ui.setupUi(this);
}