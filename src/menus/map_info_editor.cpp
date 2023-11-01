#include "map_info_editor.h"

#include "globals.h"
#include <map_global.h>
#include <filesystem>

namespace fs = std::filesystem;

import Utilities;

MapInfoEditor::MapInfoEditor(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	// Description Tab
	ui.name->setText(QString::fromStdString(map->trigger_strings.string(map->info.name)));
	ui.suggestedPlayers->setText(QString::fromStdString(map->trigger_strings.string(map->info.suggested_players)));
	ui.description->setPlainText(QString::fromStdString(map->trigger_strings.string(map->info.description)));
	ui.author->setText(QString::fromStdString(map->trigger_strings.string(map->info.author)));

	ui.mapVersion->setText(QString::number(map->info.map_version));
	ui.editorVersion->setText(QString::number(map->info.editor_version));

	// Loading Screen Tab
	for (auto&&[key, value] : world_edit_data.section("LoadingScreens")) {
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

	for (auto&&[key, value] : world_edit_data.section("LoadingScreens")) {
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

	ui.loadingScreenTitle->setText(QString::fromStdString(map->trigger_strings.string(map->info.loading_screen_title)));
	ui.loadingScreenSubtitle->setText(QString::fromStdString(map->trigger_strings.string(map->info.loading_screen_subtitle)));
	ui.loadingScreenText->setPlainText(QString::fromStdString(map->trigger_strings.string(map->info.loading_screen_text)));

	// Options Tab
	ui.meleeMap->setChecked(map->info.melee_map);
	ui.hideMinimapPreview->setChecked(map->info.hide_minimap_preview);
	ui.maskedPartiallyVisible->setChecked(map->info.masked_area_partially_visible);
	ui.cliffWaves->setChecked(map->info.cliff_shore_waves);
	ui.rollingShoreWaves->setChecked(map->info.rolling_shore_waves);

	ui.terrainFogBox->setChecked(map->info.fog_style != 0);
	ui.fogStyle->setCurrentIndex(map->info.fog_style);
	ui.fogZStart->setValue(map->info.fog_start_z_height);
	ui.fogZEnd->setValue(map->info.fog_end_z_height);
	ui.fogDensity->setValue(map->info.fog_density);
	ui.fogColor->setColor(QColor(map->info.fog_color.r, map->info.fog_color.g, map->info.fog_color.b));

	ui.waterTinting->setChecked(map->info.water_tinting);
	ui.waterColor->setColor(QColor(map->info.water_color.r, map->info.water_color.g, map->info.water_color.b));

	ui.globalWeather->setChecked(map->info.weather_id != 0);

	// Global Weather
	slk::SLK weather_slk("TerrainArt/Weather.slk");
	weather_slk.substitute(world_edit_strings, "WorldEditStrings");

	ui.globalWeather->setChecked(map->info.weather_id != 0);
	for (size_t i = 1; i < weather_slk.rows(); i++) {
		ui.globalWeatherCombo->addItem(QString::fromStdString(weather_slk.data("name", i)), QString::fromStdString(weather_slk.data("effectid", i)));
	}
	std::string weather_id = { reinterpret_cast<char*>(&map->info.weather_id), 4 };
	ui.globalWeatherCombo->setCurrentText(QString::fromStdString(weather_slk.data("name", weather_id)));

	// Custom Sound
	slk::SLK environment_sounds_slk("UI/SoundInfo/EnvironmentSounds.slk");
	environment_sounds_slk.substitute(world_edit_strings, "WorldEditStrings");

	ui.customSound->setChecked(!map->info.custom_sound_environment.empty());
	for (size_t i = 1; i < environment_sounds_slk.rows(); i++) {
		ui.customSoundCombo->addItem(QString::fromStdString(environment_sounds_slk.data("displaytext", i)), QString::fromStdString(environment_sounds_slk.data("environmenttype", i)));
	}
	ui.customSoundCombo->setCurrentText(QString::fromStdString(environment_sounds_slk.data("displaytext", map->info.custom_sound_environment)));

	// Custom Lighting
	for (auto&& [key, value] : world_edit_data.section("TileSets")) {
		ui.customLightingCombo->addItem(QString::fromStdString(value[0]), key.front());

		if (key == std::string(&map->info.custom_light_tileset, 1)) {
			ui.customLightingCombo->setCurrentIndex(ui.customLightingCombo->count() - 1);
		}
	}
	ui.customLighting->setChecked(map->info.custom_light_tileset != 0);

	ui.itemClassification->setChecked(map->info.item_classification);
	ui.gameDataSet->setCurrentIndex(map->info.game_data_set);


	connect(ui.buttonBox, &QDialogButtonBox::accepted, [&]() {
		save();
		emit accept();
		close();
	});

	connect(ui.buttonBox, &QDialogButtonBox::rejected, [&]() {
		emit reject();
		close();
	});

	show();
}

void MapInfoEditor::save() const {
	// Description Tab
	map->trigger_strings.set_string(map->info.name, ui.name->text().toStdString());
	map->trigger_strings.set_string(map->info.author, ui.author->text().toStdString());
	map->trigger_strings.set_string(map->info.description, ui.description->toPlainText().toStdString());
	map->trigger_strings.set_string(map->info.suggested_players, ui.suggestedPlayers->text().toStdString());

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

	// Options Tab
	map->info.melee_map = ui.meleeMap->isChecked();
	map->info.hide_minimap_preview = ui.hideMinimapPreview->isChecked();
	map->info.masked_area_partially_visible = ui.maskedPartiallyVisible->isChecked();
	map->info.cliff_shore_waves = ui.cliffWaves->isChecked();
	map->info.rolling_shore_waves = ui.rollingShoreWaves->isChecked();
	map->info.item_classification = ui.itemClassification->isChecked();

	map->info.fog_style = ui.fogStyle->currentIndex();
	map->info.fog_start_z_height = ui.fogZStart->value();
	map->info.fog_end_z_height = ui.fogZEnd->value();
	map->info.fog_density = ui.fogDensity->value();
	map->info.fog_color = ui.fogColor->get_glm_color();

	// Global Weather
	if (ui.globalWeather->isChecked()) {
		map->info.weather_id = *reinterpret_cast<int*>(ui.globalWeatherCombo->currentData().toString().toStdString().data());
	} else {
		map->info.weather_id = 0;
	}

	// Custom Sound
	if (ui.customSound->isChecked()) {
		map->info.custom_sound_environment = ui.customSoundCombo->currentData().toString().toStdString();
	} else {
		map->info.custom_sound_environment = "";
	}

	// Custom Lighting
	if (!ui.customLighting->isChecked()) {
		map->info.custom_light_tileset = 0;
	} else {
		map->info.custom_light_tileset = ui.customLightingCombo->currentData().toChar().toLatin1();
	}

	map->info.water_tinting = ui.waterTinting->isChecked();
	map->info.water_color = ui.waterColor->get_glm_color();
}