#include "map_info_editor.h"

import MapGlobal;

namespace fs = std::filesystem;

void MapInfoEditor::setup_description() {
	ui.name->setText(QString::fromUtf8(map->trigger_strings.string(map->info.name)));
	ui.suggestedPlayers->setText(QString::fromUtf8(map->trigger_strings.string(map->info.suggested_players)));
	ui.description->setPlainText(QString::fromUtf8(map->trigger_strings.string(map->info.description)));
	ui.author->setText(QString::fromUtf8(map->trigger_strings.string(map->info.author)));

	ui.mapVersion->setText(QString::number(map->info.map_version));
	ui.editorVersion->setText(QString::number(map->info.editor_version));
}

bool MapInfoEditor::save_description() const {
	map->trigger_strings.set_string(map->info.name, ui.name->text().toStdString());
	map->trigger_strings.set_string(map->info.author, ui.author->text().toStdString());
	map->trigger_strings.set_string(map->info.description, ui.description->toPlainText().toStdString());
	map->trigger_strings.set_string(map->info.suggested_players, ui.suggestedPlayers->text().toStdString());
	return true;
}
