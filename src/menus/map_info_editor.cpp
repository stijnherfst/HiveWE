#include "map_info_editor.h"

#include <QMessageBox>

import std;
import SLK;
import Utilities;
import MapGlobal;
import Globals;

namespace fs = std::filesystem;

MapInfoEditor::MapInfoEditor(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	// Description Tab
	ui.name->setText(QString::fromUtf8(map->trigger_strings.string(map->info.name)));
	ui.suggestedPlayers->setText(QString::fromUtf8(map->trigger_strings.string(map->info.suggested_players)));
	ui.description->setPlainText(QString::fromUtf8(map->trigger_strings.string(map->info.description)));
	ui.author->setText(QString::fromUtf8(map->trigger_strings.string(map->info.author)));

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

	ui.loadingScreenTitle->setText(QString::fromUtf8(map->trigger_strings.string(map->info.loading_screen_title)));
	ui.loadingScreenSubtitle->setText(QString::fromUtf8(map->trigger_strings.string(map->info.loading_screen_subtitle)));
	ui.loadingScreenText->setPlainText(QString::fromUtf8(map->trigger_strings.string(map->info.loading_screen_text)));

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
		ui.globalWeatherCombo->addItem(QString::fromUtf8(weather_slk.data<std::string_view>("name", i)), QString::fromUtf8(weather_slk.data<std::string_view>("effectid", i)));
	}
	std::string weather_id = { reinterpret_cast<char*>(&map->info.weather_id), 4 };
	ui.globalWeatherCombo->setCurrentText(QString::fromUtf8(weather_slk.data<std::string_view>("name", weather_id)));

	// Custom Sound
	slk::SLK environment_sounds_slk("UI/SoundInfo/EnvironmentSounds.slk");
	environment_sounds_slk.substitute(world_edit_strings, "WorldEditStrings");

	ui.customSound->setChecked(!map->info.custom_sound_environment.empty());
	for (size_t i = 1; i < environment_sounds_slk.rows(); i++) {
		ui.customSoundCombo->addItem(QString::fromUtf8(environment_sounds_slk.data<std::string_view>("displaytext", i)), QString::fromUtf8(environment_sounds_slk.data<std::string_view>("environmenttype", i)));
	}
	ui.customSoundCombo->setCurrentText(QString::fromUtf8(environment_sounds_slk.data<std::string_view>("displaytext", map->info.custom_sound_environment)));

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

	// Map size tab
	// initialise values
	oldMapBottomLeft.x = 0;
	oldMapBottomLeft.y = 0;
	oldMapTopRight.x = map->terrain.width - 1;
	oldMapTopRight.y = map->terrain.height - 1;
	newMapTopRight = oldMapTopRight;
	newMapBottomLeft = oldMapBottomLeft;

	oldPlayableBottomLeft.x = map->info.camera_complements[0];
	oldPlayableBottomLeft.y = map->info.camera_complements[2];
	oldPlayableTopRight.x = oldMapTopRight.x - map->info.camera_complements[1];
	oldPlayableTopRight.y = oldMapTopRight.y - map->info.camera_complements[3];
	newPlayableBottomLeft = oldPlayableBottomLeft;
	newPlayableTopRight = oldPlayableTopRight;

	updateSizeDisplays();

	// connect arrow buttons to map size
	connect(ui.mapBoundsLeftDec, &QPushButton::clicked, [this]() { adjustBounds(1, 0, 0, 0); });
	connect(ui.mapBoundsLeftInc, &QPushButton::clicked, [this]() { adjustBounds(-1, 0, 0, 0); });
	connect(ui.mapBoundsRightDec, &QPushButton::clicked, [this]() { adjustBounds(0, -1, 0, 0); });
	connect(ui.mapBoundsRightInc, &QPushButton::clicked, [this]() { adjustBounds(0, 1, 0, 0); });
	connect(ui.mapBoundsTopDec, &QPushButton::clicked, [this]() { adjustBounds(0, 0, -1, 0); });
	connect(ui.mapBoundsTopInc, &QPushButton::clicked, [this]() { adjustBounds(0, 0, 1, 0); });
	connect(ui.mapBoundsBottomDec, &QPushButton::clicked, [this]() { adjustBounds(0, 0, 0, 1); });
	connect(ui.mapBoundsBottomInc, &QPushButton::clicked, [this]() { adjustBounds(0, 0, 0, -1); });

	// reset camera bounds (unplayable area) to default
	connect(ui.resetCameraBounds, &QPushButton::clicked, [this]() {
		newPlayableBottomLeft.x = newMapBottomLeft.x + 6;
		newPlayableBottomLeft.y = newMapBottomLeft.y + 4;
		newPlayableTopRight.x = newMapTopRight.x - 6;
		newPlayableTopRight.y = newMapTopRight.y - 8;
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

	// Map size and camera bounds
	bool changedMapSize = (newMapBottomLeft != oldMapBottomLeft) || (newMapTopRight != oldMapTopRight);
	bool changedPlayableSize = (newPlayableBottomLeft != oldPlayableBottomLeft) || (newPlayableTopRight != oldPlayableTopRight);

	if (changedMapSize || changedPlayableSize) {
		int newWidth = newMapTopRight.x - newMapBottomLeft.x;
		int newHeight = newMapTopRight.y - newMapBottomLeft.y;

		int newPlayableWidth = newPlayableTopRight.x - newPlayableBottomLeft.x;
		int newPlayableHeight = newPlayableTopRight.y - newPlayableBottomLeft.y;

		if (newWidth < 32 || newWidth > 480 || newHeight < 32 || newHeight > 480) {
			QMessageBox::critical(const_cast<MapInfoEditor*>(this), "Invalid Map Size", QString("Map dimensions must be between 32 and 480.\nNew size would be: %1 x %2").arg(newWidth).arg(newHeight));
		}
		else if (newWidth % 32 != 0 || newHeight % 32 != 0) {
			QMessageBox::critical(const_cast<MapInfoEditor*>(this), "Invalid Map Size", QString("Map dimensions must be divisible by 32.\nNew size would be: %1 x %2").arg(newWidth).arg(newHeight));
		}
		else if (newPlayableWidth < 9 || newPlayableHeight < 5) {
			QMessageBox::critical(const_cast<MapInfoEditor*>(this), "Invalid Playable Area", QString("Playable area must be at least 9x5.\nNew playable size would be: %1 x %2").arg(newPlayableWidth).arg(newPlayableHeight));
		}
		else {
			// to make this simpler, we first get rid of old bounduaries
			if (changedMapSize || changedPlayableSize) {
				map->set_playable_area(0, 0, 0, 0);
			}

			// resize the terrain
			if (changedMapSize) {
				int deltaLeft = oldMapBottomLeft.x - newMapBottomLeft.x;
				int deltaRight = newMapTopRight.x - oldMapTopRight.x;
				int deltaBottom = oldMapBottomLeft.y - newMapBottomLeft.y;
				int deltaTop = newMapTopRight.y - oldMapTopRight.y;
				map->resize(deltaLeft, deltaRight, deltaTop, deltaBottom);
			}

			// apply camera bounds changes
			if (changedMapSize || changedPlayableSize) {
				int unplayableLeft = newPlayableBottomLeft.x - newMapBottomLeft.x;
				int unplayableRight = newMapTopRight.x - newPlayableTopRight.x;
				int unplayableBottom = newPlayableBottomLeft.y - newMapBottomLeft.y;
				int unplayableTop = newMapTopRight.y - newPlayableTopRight.y;
				map->set_playable_area(unplayableLeft, unplayableRight, unplayableTop, unplayableBottom);
			}
		}
	}
}

void MapInfoEditor::updateSizeDisplays() {
	// updates the GUI when user presses arrows to change map size
	int newWidth = newMapTopRight.x - newMapBottomLeft.x;
	int newHeight = newMapTopRight.y - newMapBottomLeft.y;

	int newPlayableWidth = newPlayableTopRight.x - newPlayableBottomLeft.x;
	int newPlayableHeight = newPlayableTopRight.y - newPlayableBottomLeft.y;

	// update  map size labels
	ui.mapSizeFull->setText(QString::fromStdString(std::format("{} x {}", newWidth, newHeight)));
	ui.mapSizePlayable->setText(QString::fromStdString(std::format("{} x {}", newPlayableWidth, newPlayableHeight)));
}

void MapInfoEditor::adjustBounds(int deltaLeft, int deltaRight, int deltaTop, int deltaBottom) {
	int newWidth = newMapTopRight.x - newMapBottomLeft.x;
	int newHeight = newMapTopRight.y - newMapBottomLeft.y;

	// handle terrain size change
	if (ui.modifyMapBounds->isChecked()) {
		// vanilla editor behaviour - changing map is 4 times faster
		deltaLeft *= 4;
		deltaRight *= 4;
		deltaTop *= 4;
		deltaBottom *= 4;

		// accept the adjustment if the map is within acceptable bounds
		newWidth += deltaLeft + deltaRight;
		newHeight += deltaTop + deltaBottom;

		// bounds change
		if (newWidth >= 32 and newWidth <= 480 and newHeight >= 32 and newHeight <= 480) {
			newMapBottomLeft.x -= deltaLeft;
			newMapBottomLeft.y -= deltaBottom;
			newMapTopRight.x += deltaRight;
			newMapTopRight.y += deltaTop;
		}
	}

	// handle playable area change
	if (ui.modifyCameraBounds->isChecked()) { 
		newPlayableBottomLeft.x -= deltaLeft;
		newPlayableBottomLeft.y -= deltaBottom;
		newPlayableTopRight.x += deltaRight;
		newPlayableTopRight.y += deltaTop;
	}

	// ensure playable area stays within map bounds
	newPlayableBottomLeft.x = std::max(newPlayableBottomLeft.x, newMapBottomLeft.x);
	newPlayableBottomLeft.y = std::max(newPlayableBottomLeft.y, newMapBottomLeft.y);
	newPlayableTopRight.x = std::min(newPlayableTopRight.x, newMapTopRight.x);
	newPlayableTopRight.y = std::min(newPlayableTopRight.y, newMapTopRight.y);

	// ensure a minimum 9x5 playable area size
	int playableWidth = newPlayableTopRight.x - newPlayableBottomLeft.x;
	int playableHeight = newPlayableTopRight.y - newPlayableBottomLeft.y;

	if (playableWidth < 9) {
		int deficit = 9 - playableWidth;
		// expand playable area on the opposite side to compensate
		if (deltaLeft != 0) {
			// left was adjusted, expand right
			newPlayableTopRight.x += deficit;
		} else if (deltaRight != 0) {
			// right was adjusted, expand left
			newPlayableBottomLeft.x -= deficit;
		}
	}

	if (playableHeight < 5) {
		int deficit = 5 - playableHeight;
		// expand playable area on the opposite side to compensate
		if (deltaBottom != 0) {
			// bottom was adjusted, expand top
			newPlayableTopRight.y += deficit;
		} else if (deltaTop != 0) {
			// top was adjusted, expand bottom
			newPlayableBottomLeft.y -= deficit;
		}
	}

	updateSizeDisplays();
}
