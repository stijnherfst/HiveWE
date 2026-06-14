#include "map_info_editor.h"

import MapGlobal;
import Globals;
import SLK;

namespace fs = std::filesystem;

void MapInfoEditor::setup_options() {
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
	ui.waterColor->setColor(QColor(map->info.water_color.r, map->info.water_color.g, map->info.water_color.b, map->info.water_color.a));

	ui.globalWeather->setChecked(map->info.weather_id != 0);

	// Global Weather
	slk::SLK weather_slk("TerrainArt/Weather.slk");
	weather_slk.substitute(world_edit_strings, "WorldEditStrings");

	ui.globalWeather->setChecked(map->info.weather_id != 0);
	for (size_t i = 1; i < weather_slk.rows(); i++) {
		ui.globalWeatherCombo->addItem(
			QString::fromUtf8(weather_slk.data<std::string_view>("name", i)),
			QString::fromUtf8(weather_slk.data<std::string_view>("effectid", i))
		);
	}
	std::string weather_id = {reinterpret_cast<char*>(&map->info.weather_id), 4};
	ui.globalWeatherCombo->setCurrentText(QString::fromUtf8(weather_slk.data<std::string_view>("name", weather_id)));

	// Custom Sound
	slk::SLK environment_sounds_slk("UI/SoundInfo/EnvironmentSounds.slk");
	environment_sounds_slk.substitute(world_edit_strings, "WorldEditStrings");

	ui.customSound->setChecked(!map->info.custom_sound_environment.empty());
	for (const auto& [row_key, i] : environment_sounds_slk.row_headers) {
		ui.customSoundCombo->addItem(
			QString::fromUtf8(environment_sounds_slk.data<std::string_view>("displaytext", i)),
			QString::fromUtf8(row_key)
		);
	}
	ui.customSoundCombo->setCurrentText(
		QString::fromUtf8(environment_sounds_slk.data<std::string_view>("displaytext", map->info.custom_sound_environment))
	);

	// Custom Lighting and Ambiance
	for (const auto& [key, tileset] : map->tilesets.tilesets()) {
		ui.customLightingCombo->addItem(QString::fromStdString(tileset.name), QChar(key));
		ui.customAmbianceCombo->addItem(QString::fromStdString(tileset.name), QChar(key));

		if (key == map->info.custom_light_tileset) {
			ui.customLightingCombo->setCurrentIndex(ui.customLightingCombo->count() - 1);
		}

		if (key == map->info.custom_ambience_tileset) {
			ui.customAmbianceCombo->setCurrentIndex(ui.customAmbianceCombo->count() - 1);
		}
	}
	ui.customLighting->setChecked(map->info.custom_light_tileset != 0);
	ui.customAmbiance->setChecked(map->info.custom_ambience_tileset != 0);

	ui.itemClassification->setChecked(map->info.item_classification);
	ui.gameDataSet->setCurrentIndex(map->info.game_data_set);
}

bool MapInfoEditor::save_options() const {
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

	// Custom Ambiance
	if (!ui.customAmbiance->isChecked()) {
		map->info.custom_ambience_tileset = 0;
	} else {
		map->info.custom_ambience_tileset = ui.customAmbianceCombo->currentData().toChar().toLatin1();
	}

	map->info.water_tinting = ui.waterTinting->isChecked();
	map->info.water_color = ui.waterColor->get_glm_color();
	return true;
}
