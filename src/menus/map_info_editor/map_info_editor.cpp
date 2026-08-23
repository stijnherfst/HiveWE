#include "map_info_editor.h"

#include <QMessageBox>
#include <QPainter>

import std;
import SLK;
import Utilities;
import Globals;
import Tileset;

namespace fs = std::filesystem;

MapInfoEditor::MapInfoEditor(QWidget* parent, Map& map)
	: QDialog(parent), map(map), info(map.info), trigger_strings(map.trigger_strings), terrain(map.terrain), tilesets(map.tilesets) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	setup_description(info, trigger_strings);
	setup_loading_screen(info, trigger_strings, map.filesystem_path);
	setup_options(info, tilesets);
	setup_map_size(terrain, info);

	connect(ui.buttonBox, &QDialogButtonBox::accepted, [this]() {
		save();
		emit accept();
		close();
	});

	connect(ui.buttonBox, &QDialogButtonBox::rejected, [this]() {
		emit reject();
		close();
	});

	show();
}

void MapInfoEditor::save() const {
	save_description(info, trigger_strings);
	save_loading_screen(info, trigger_strings);
	save_options(info);
	save_map_size(map);
}
