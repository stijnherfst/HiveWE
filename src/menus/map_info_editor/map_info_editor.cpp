#include "map_info_editor.h"

#include <QMessageBox>
#include <QPainter>

import <filesystem>;
import SLK;
import Utilities;
import MapGlobal;
import Globals;
import Tileset;

namespace fs = std::filesystem;

MapInfoEditor::MapInfoEditor(QWidget* parent) : QDialog(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	const MapInfo& info = map->info;
	setup_description(info, map->trigger_strings);
	setup_loading_screen(info, map->trigger_strings, map->filesystem_path);
	setup_options(info, map->tilesets);
	setup_map_size(map->terrain, info);

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

	saved &= save_description(map->info, map->trigger_strings);
	saved &= save_loading_screen(map->info, map->trigger_strings);
	saved &= save_options(map->info);
	saved &= save_map_size(*map);

	return saved;
}
