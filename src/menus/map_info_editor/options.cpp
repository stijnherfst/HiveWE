#include "map_info_editor.h"

import Globals;
import SLK;

namespace fs = std::filesystem;

void MapInfoEditor::setup_options(const MapInfo& info, const TilesetData& tilesets) {
	ui.meleeMap->setChecked(info.melee_map);
	ui.hideMinimapPreview->setChecked(info.hide_minimap_preview);
	ui.maskedPartiallyVisible->setChecked(info.masked_area_partially_visible);
	ui.cliffWaves->setChecked(info.cliff_shore_waves);
	ui.rollingShoreWaves->setChecked(info.rolling_shore_waves);

	ui.terrainFogBox->setChecked(info.fog_style != 0);
	ui.fogStyle->setCurrentIndex(info.fog_style);
	ui.fogZStart->setValue(info.fog_start_z_height);
	ui.fogZEnd->setValue(info.fog_end_z_height);
	ui.fogDensity->setValue(info.fog_density);
	ui.fogColor->setColor(QColor(info.fog_color.r, info.fog_color.g, info.fog_color.b));

	ui.waterTinting->setChecked(info.water_tinting);
	ui.waterColor->setColor(QColor(info.water_color.r, info.water_color.g, info.water_color.b, info.water_color.a));

	ui.globalWeather->setChecked(info.weather_id != 0);

	// Global Weather
	slk::SLK weather_slk("TerrainArt/Weather.slk");
	weather_slk.substitute(world_edit_strings, "WorldEditStrings");

	ui.globalWeather->setChecked(info.weather_id != 0);
	for (size_t i = 1; i < weather_slk.rows(); i++) {
		ui.globalWeatherCombo->addItem(
			QString::fromUtf8(weather_slk.data<std::string_view>("name", i)),
			QString::fromUtf8(weather_slk.data<std::string_view>("effectid", i))
		);
	}
	std::string weather_id = {reinterpret_cast<const char*>(&info.weather_id), 4};
	ui.globalWeatherCombo->setCurrentText(QString::fromUtf8(weather_slk.data<std::string_view>("name", weather_id)));

	// Custom Sound
	slk::SLK environment_sounds_slk("UI/SoundInfo/EnvironmentSounds.slk");
	environment_sounds_slk.substitute(world_edit_strings, "WorldEditStrings");

	ui.customSound->setChecked(!info.custom_sound_environment.empty());
	for (const auto& [row_key, i] : environment_sounds_slk.row_headers) {
		ui.customSoundCombo->addItem(
			QString::fromUtf8(environment_sounds_slk.data<std::string_view>("displaytext", i)),
			QString::fromUtf8(row_key)
		);
	}
	ui.customSoundCombo->setCurrentText(
		QString::fromUtf8(environment_sounds_slk.data<std::string_view>("displaytext", info.custom_sound_environment))
	);

	// Custom Lighting and Ambiance
	for (const auto& [key, tileset] : tilesets.tilesets()) {
		ui.customLightingCombo->addItem(QString::fromStdString(tileset.name), QChar(key));
		ui.customAmbianceCombo->addItem(QString::fromStdString(tileset.name), QChar(key));

		if (key == info.custom_light_tileset) {
			ui.customLightingCombo->setCurrentIndex(ui.customLightingCombo->count() - 1);
		}

		if (key == info.custom_ambience_tileset) {
			ui.customAmbianceCombo->setCurrentIndex(ui.customAmbianceCombo->count() - 1);
		}
	}
	ui.customLighting->setChecked(info.custom_light_tileset != 0);
	ui.customAmbiance->setChecked(info.custom_ambience_tileset != 0);

	ui.itemClassification->setChecked(info.item_classification);
	ui.gameDataSet->setCurrentIndex(info.game_data_set);
}

bool MapInfoEditor::save_options(MapInfo& info) const {
	info.melee_map = ui.meleeMap->isChecked();
	info.hide_minimap_preview = ui.hideMinimapPreview->isChecked();
	info.masked_area_partially_visible = ui.maskedPartiallyVisible->isChecked();
	info.cliff_shore_waves = ui.cliffWaves->isChecked();
	info.rolling_shore_waves = ui.rollingShoreWaves->isChecked();
	info.item_classification = ui.itemClassification->isChecked();

	info.fog_style = ui.fogStyle->currentIndex();
	info.fog_start_z_height = ui.fogZStart->value();
	info.fog_end_z_height = ui.fogZEnd->value();
	info.fog_density = ui.fogDensity->value();
	info.fog_color = ui.fogColor->get_glm_color();

	// Global Weather
	if (ui.globalWeather->isChecked()) {
		info.weather_id = *reinterpret_cast<int*>(ui.globalWeatherCombo->currentData().toString().toStdString().data());
	} else {
		info.weather_id = 0;
	}

	// Custom Sound
	if (ui.customSound->isChecked()) {
		info.custom_sound_environment = ui.customSoundCombo->currentData().toString().toStdString();
	} else {
		info.custom_sound_environment = "";
	}

	// Custom Lighting
	if (!ui.customLighting->isChecked()) {
		info.custom_light_tileset = 0;
	} else {
		info.custom_light_tileset = ui.customLightingCombo->currentData().toChar().toLatin1();
	}

	// Custom Ambiance
	if (!ui.customAmbiance->isChecked()) {
		info.custom_ambience_tileset = 0;
	} else {
		info.custom_ambience_tileset = ui.customAmbianceCombo->currentData().toChar().toLatin1();
	}

	info.water_tinting = ui.waterTinting->isChecked();
	info.water_color = ui.waterColor->get_glm_color();
	return true;
}
