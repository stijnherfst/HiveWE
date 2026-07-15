#include "map_info_editor.h"

import MapGlobal;
import Globals;
import Utilities;

namespace fs = std::filesystem;

void MapInfoEditor::setup_loading_screen() {
	for (const auto& [key, value] : world_edit_data.section("LoadingScreens")) {
		if (key == "NumScreens") {
			continue;
		}
		ui.campaignLoadingScreen->addItem(QString::fromStdString(value[1]));
	}

	for (const auto& entry : fs::recursive_directory_iterator(map->filesystem_path)) {
		if (entry.is_directory()) {
			continue;
		}
		if (to_lowercase_copy(entry.path().extension().string()) == ".mdx") {
			ui.importedLoadingScreen->addItem(QString::fromStdString(entry.path().lexically_relative(map->filesystem_path).string()));
		}
	}

	for (const auto& [key, value] : world_edit_data.section("LoadingScreens")) {
		if (key == "NumScreens") {
			continue;
		}
		ui.campaignLoadingScreen->addItem(QString::fromStdString(value[1]));
	}

	if (map->info.loading_screen_model.empty() && map->info.loading_screen_number == -1) {
		ui.useDefaultLoadingScreen->setChecked(true);
	} else if (!map->info.loading_screen_model.empty() && map->info.loading_screen_number == -1) {
		ui.useImportedLoadingScreen->setChecked(true);
		ui.importedLoadingScreen->setCurrentText(QString::fromStdString(map->info.loading_screen_model));
	} else {
		ui.useCampaignLoadingScreen->setChecked(true);
		ui.campaignLoadingScreen->setCurrentIndex(map->info.loading_screen_number);
	}

	ui.loadingScreenTitle->setText(QString::fromUtf8(map->trigger_strings.string(map->info.loading_screen_title)));
	ui.loadingScreenSubtitle->setText(QString::fromUtf8(map->trigger_strings.string(map->info.loading_screen_subtitle)));
	ui.loadingScreenText->setPlainText(QString::fromUtf8(map->trigger_strings.string(map->info.loading_screen_text)));
}

bool MapInfoEditor::save_loading_screen() const {
	if (ui.useDefaultLoadingScreen->isChecked()) {
		map->info.loading_screen_model = "";
		map->info.loading_screen_number = -1;
	} else if (ui.useImportedLoadingScreen->isChecked()) {
		map->info.loading_screen_model = ui.importedLoadingScreen->currentText().toStdString();
		map->info.loading_screen_number = -1;
	} else {
		map->info.loading_screen_model = "";
		map->info.loading_screen_number = ui.campaignLoadingScreen->currentIndex();
	}

	map->trigger_strings.set_string(map->info.loading_screen_text, ui.loadingScreenText->toPlainText().toStdString());
	map->trigger_strings.set_string(map->info.loading_screen_title, ui.loadingScreenTitle->text().toStdString());
	map->trigger_strings.set_string(map->info.loading_screen_subtitle, ui.loadingScreenSubtitle->text().toStdString());

	map->info.game_data_set = ui.gameDataSet->currentIndex();

	// Prologue?
	return true;
}
