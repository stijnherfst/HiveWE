#include "stdafx.h"

MapInfoEditor::MapInfoEditor(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	connect(ui.buttonBox, &QDialogButtonBox::accepted, [&]() {
		save();
		emit accept();
		close();
	});

	connect(ui.buttonBox, &QDialogButtonBox::rejected, [&]() {
		emit reject();
		close();
	});

	// Description Tab
	ui.name->setText(QString::fromStdString(map.trigger_strings.strings[map.info.name]));
	ui.suggestedPlayers->setText(QString::fromStdString(map.trigger_strings.strings[map.info.suggested_players]));
	ui.description->setPlainText(QString::fromStdString(map.trigger_strings.strings[map.info.description]));
	ui.author->setText(QString::fromStdString(map.trigger_strings.strings[map.info.author]));

	ui.mapVersion->setText(QString::number(map.info.map_version));
	ui.editorVersion->setText(QString::number(map.info.editor_version));

	// Loading Screen Tab
	for (auto&&[key, value] : world_edit_data.section("LoadingScreens")) {
		if (key == "NumScreens") {
			continue;
		}
		ui.campaignLoadingScreen->addItem(QString::fromStdString(split(value, ',')[1]));
	}

	for (auto&& i : map.imports.find([](const ImportItem& item) { return item.full_path.extension() == ".mdx"; })) {
		ui.importedLoadingScreen->addItem(QString::fromStdString(i.get().full_path.string()));
	}

	for (auto&&[key, value] : world_edit_data.section("LoadingScreens")) {
		if (key == "NumScreens") {
			continue;
		}
		ui.campaignLoadingScreen->addItem(QString::fromStdString(split(value, ',')[1]));
	}

	if (map.info.loading_screen_model.empty() && map.info.loading_screen_number == -1) {
		ui.useDefaultLoadingScreen->setChecked(true);
	} else if (!map.info.loading_screen_model.empty() && map.info.loading_screen_number == -1) {
		ui.useImportedLoadingScreen->setChecked(true);
		ui.importedLoadingScreen->setCurrentText(QString::fromStdString(map.info.loading_screen_model));
	} else {
		ui.useCampaignLoadingScreen->setChecked(true);
		ui.campaignLoadingScreen->setCurrentIndex(map.info.loading_screen_number);
	}

	ui.loadingScreenTitle->setText(QString::fromStdString(map.trigger_strings.strings[map.info.loading_screen_title]));
	ui.loadingScreenSubtitle->setText(QString::fromStdString(map.trigger_strings.strings[map.info.loading_screen_subtitle]));
	ui.loadingScreenText->setPlainText(QString::fromStdString(map.trigger_strings.strings[map.info.loading_screen_text]));

	// Options Tab
	ui.meleeMap->setChecked(map.info.melee_map);
	ui.hideMinimapPreview->setChecked(map.info.hide_minimap_preview);
	ui.maskedPartiallyVisible->setChecked(map.info.masked_area_partially_visible);
	ui.cliffWaves->setChecked(map.info.cliff_shore_waves);
	ui.rollingShoreWaves->setChecked(map.info.rolling_shore_waves);

	ui.terrainFogBox->setChecked(map.info.fog_style != 0);
	ui.fogStyle->setCurrentIndex(map.info.fog_style);
	ui.fogZStart->setValue(map.info.fog_start_z_height);
	ui.fogZEnd->setValue(map.info.fog_end_z_height);
	ui.fogDensity->setValue(map.info.fog_density);
	ui.fogColor->setColor(QColor(map.info.fog_color.r, map.info.fog_color.g, map.info.fog_color.b));

	ui.waterTinting->setChecked(map.info.water_tinting);
	ui.waterColor->setColor(QColor(map.info.water_color.r, map.info.water_color.g, map.info.water_color.b));

	ui.globalWeather->setChecked(map.info.weather_id != 0);

	ui.customLighting->setChecked(!map.info.custom_sound_environment.empty());
	slk::SLK environment_sounds_slk("UI/SoundInfo/EnvironmentSounds.slk");
	environment_sounds_slk.merge(ini::INI("UI/WorldEditStrings.txt"), "WorldEditStrings");

	for (int i = 1; i < environment_sounds_slk.rows; i++) {
		ui.customLightingCombo->addItem(QString::fromStdString(environment_sounds_slk.data("DisplayText", i)));
	}
	ui.customLightingCombo->setCurrentText(QString::fromStdString(environment_sounds_slk.data("DisplayText", map.info.custom_sound_environment)));

	ui.gameDataSet->setCurrentIndex(map.info.game_data_set);

	ui.itemClassification->setChecked(map.info.item_classification);

	show();
}

void MapInfoEditor::save() const {
	// Description Tab
	map.trigger_strings.strings[map.info.name] = ui.name->text().toStdString();
	map.trigger_strings.strings[map.info.suggested_players] = ui.suggestedPlayers->text().toStdString();
	map.trigger_strings.strings[map.info.description] = ui.description->toPlainText().toStdString();
	map.trigger_strings.strings[map.info.author] = ui.author->text().toStdString();

	if (ui.useDefaultLoadingScreen->isChecked()) {
		map.info.loading_screen_model = "";
		map.info.loading_screen_number = -1;
	} else if (ui.useImportedLoadingScreen->isChecked()) {
		map.info.loading_screen_model = ui.importedLoadingScreen->currentText().toStdString();
		map.info.loading_screen_number = -1;
	} else {
		map.info.loading_screen_model = "";
		map.info.loading_screen_number = ui.campaignLoadingScreen->currentIndex();
	}

	map.trigger_strings.strings[map.info.loading_screen_title] = ui.loadingScreenTitle->text().toStdString();
	map.trigger_strings.strings[map.info.loading_screen_subtitle] = ui.loadingScreenSubtitle->text().toStdString();
	map.trigger_strings.strings[map.info.loading_screen_text] = ui.loadingScreenText->toPlainText().toStdString();

	// Options Tab
	map.info.melee_map = ui.meleeMap->isChecked();
	map.info.hide_minimap_preview = ui.hideMinimapPreview->isChecked();
	map.info.masked_area_partially_visible = ui.maskedPartiallyVisible->isChecked();
	map.info.cliff_shore_waves = ui.cliffWaves->isChecked();
	map.info.rolling_shore_waves = ui.rollingShoreWaves->isChecked();
	map.info.item_classification = ui.itemClassification->isChecked();

	map.info.fog_style = ui.fogStyle->currentIndex();
	map.info.fog_start_z_height = ui.fogZStart->value();
	map.info.fog_end_z_height = ui.fogZEnd->value();
	map.info.fog_density = ui.fogDensity->value();
	map.info.fog_color = ui.fogColor->get_glm_color();

	map.info.water_tinting = ui.waterTinting->isChecked();
	map.info.water_color = ui.waterColor->get_glm_color();


	//ui.globalWeather->setChecked(map.info.weather_id != 0);

	//ui.gameDataSet->setCurrentIndex(map.info.game_data_set);
}