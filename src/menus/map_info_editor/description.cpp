#include "map_info_editor.h"

namespace fs = std::filesystem;

void MapInfoEditor::setup_description(const MapInfo& info, const TriggerStrings& trigger_strings) {
	ui.name->setText(QString::fromUtf8(trigger_strings.string(info.name)));
	ui.suggestedPlayers->setText(QString::fromUtf8(trigger_strings.string(info.suggested_players)));
	ui.description->setPlainText(QString::fromUtf8(trigger_strings.string(info.description)));
	ui.author->setText(QString::fromUtf8(trigger_strings.string(info.author)));

	ui.mapVersion->setText(QString::number(info.map_version));
	ui.editorVersion->setText(QString::number(info.editor_version));
}

void MapInfoEditor::save_description(MapInfo& info, TriggerStrings& trigger_strings) const {
	trigger_strings.set_string(info.name, ui.name->text().toStdString());
	trigger_strings.set_string(info.author, ui.author->text().toStdString());
	trigger_strings.set_string(info.description, ui.description->toPlainText().toStdString());
	trigger_strings.set_string(info.suggested_players, ui.suggestedPlayers->text().toStdString());
}
