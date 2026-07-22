#include "map_info_editor.h"

import Globals;
import Utilities;

namespace fs = std::filesystem;

void MapInfoEditor::setup_loading_screen(const MapInfo& info, const TriggerStrings& trigger_strings, const fs::path& filesystem_path) {
	for (const auto& [key, value] : world_edit_data.section("LoadingScreens")) {
		if (key == "NumScreens") {
			continue;
		}
		ui.campaignLoadingScreen->addItem(QString::fromStdString(value[1]));
	}

	for (const auto& entry : fs::recursive_directory_iterator(filesystem_path)) {
		if (entry.is_directory()) {
			continue;
		}
		if (to_lowercase_copy(entry.path().extension().string()) == ".mdx") {
			ui.importedLoadingScreen->addItem(QString::fromStdString(entry.path().lexically_relative(filesystem_path).string()));
		}
	}

	for (const auto& [key, value] : world_edit_data.section("LoadingScreens")) {
		if (key == "NumScreens") {
			continue;
		}
		ui.campaignLoadingScreen->addItem(QString::fromStdString(value[1]));
	}

	if (info.loading_screen_model.empty() && info.loading_screen_number == -1) {
		ui.useDefaultLoadingScreen->setChecked(true);
	} else if (!info.loading_screen_model.empty() && info.loading_screen_number == -1) {
		ui.useImportedLoadingScreen->setChecked(true);
		ui.importedLoadingScreen->setCurrentText(QString::fromStdString(info.loading_screen_model));
	} else {
		ui.useCampaignLoadingScreen->setChecked(true);
		ui.campaignLoadingScreen->setCurrentIndex(info.loading_screen_number);
	}

	ui.loadingScreenTitle->setText(QString::fromUtf8(trigger_strings.string(info.loading_screen_title)));
	ui.loadingScreenSubtitle->setText(QString::fromUtf8(trigger_strings.string(info.loading_screen_subtitle)));
	ui.loadingScreenText->setPlainText(QString::fromUtf8(trigger_strings.string(info.loading_screen_text)));
}

void MapInfoEditor::save_loading_screen(MapInfo& info, TriggerStrings& trigger_strings) const {
	if (ui.useDefaultLoadingScreen->isChecked()) {
		info.loading_screen_model = "";
		info.loading_screen_number = -1;
	} else if (ui.useImportedLoadingScreen->isChecked()) {
		info.loading_screen_model = ui.importedLoadingScreen->currentText().toStdString();
		info.loading_screen_number = -1;
	} else {
		info.loading_screen_model = "";
		info.loading_screen_number = ui.campaignLoadingScreen->currentIndex();
	}

	trigger_strings.set_string(info.loading_screen_text, ui.loadingScreenText->toPlainText().toStdString());
	trigger_strings.set_string(info.loading_screen_title, ui.loadingScreenTitle->text().toStdString());
	trigger_strings.set_string(info.loading_screen_subtitle, ui.loadingScreenSubtitle->text().toStdString());

	info.game_data_set = ui.gameDataSet->currentIndex();

	// Prologue?
}
