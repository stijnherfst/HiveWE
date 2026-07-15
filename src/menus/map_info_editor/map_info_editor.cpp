#include "map_info_editor.h"

#include <QMessageBox>
#include <QPainter>

import std;
import SLK;
import Utilities;
import MapGlobal;
import Globals;
import Tileset;

namespace fs = std::filesystem;

MapInfoEditor::MapInfoEditor(QWidget* parent) : QDialog(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	setup_description();
	setup_loading_screen();
	setup_options();
	setup_map_size();

	connect(ui.buttonBox, &QDialogButtonBox::accepted, [&]() {
		if (save()) {
			emit accept();
			close();
		}
	});

	connect(ui.buttonBox, &QDialogButtonBox::rejected, [&]() {
		emit reject();
		close();
	});

	show();
}

bool MapInfoEditor::save() const {
	bool saved = true;

	saved &= save_description();
	saved &= save_loading_screen();
	saved &= save_options();
	saved &= save_map_size();

	return saved;
}
