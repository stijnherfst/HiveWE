#include "scenario_info_editor.h"

#include <QMessageBox>
#include <QPainter>
#include <cstddef>
#include <vector>
#include <string>

import std;
import SLK;
import Utilities;
import MapGlobal;
import Globals;
import Tileset;

namespace fs = std::filesystem;

ScenarioInfoEditor::ScenarioInfoEditor(QWidget* parent) : QDialog(parent) {
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    
	// Temporary local declaration of player colours in hex code.
	// TODO/Future work: expose the player colours for access from other modules
	// and preferably in float or u8 rgb values with utility/helper functions
	// for conversion to hex codes.
	// Player colours were manually calculated based on the values in data/shaders/skinned_mesh_sd.vert with
	// an exact copy in data/shaders/skinned_mesh_hd.vert, data/shaders/editable_mesh_hd.vert and
	// data/shaders/editable_mesh_sd.vert.
	// These exposed player colour could then be uploaded into the shaders to avoid duplicate
	// definitions in the shaders.
	std::vector<std::string> player_colors = {
		"ff0303",	// Red (1.000, 0.012, 0.012)
		"0042ff",	// Blue (0.000, 0.259, 1.000)
		"1be7ba",	// Teal (0.106, 0.906, 0.729)
		"550081",	// Purple (0.333, 0.000, 0.506)
		"fefc00",	// Yellow (0.996, 0.988, 0.000)
		"fe890d",	// Orange (0.996, 0.537, 0.051)
		"21bf00",	// Green (0.129, 0.749, 0.000)
		"e45caf",	// Pink (0.894, 0.361, 0.686)
		"939596",	// Gray (0.576, 0.584, 0.588)
		"7ebff1",	// Light Blue (0.494, 0.749, 0.945)
		"106247",	// Dark Green (0.063, 0.384, 0.278)
		"4f2b05",	// Brown (0.310, 0.169, 0.020)
		"9c0000",	// Maroon (0.612, 0.000, 0.000)
		"0000c3",	// Navy (0.000, 0.000, 0.765)
		"00ebff",	// Turqouise (0.000, 0.922, 1.000)
		"bd00ff",	// Violet (0.741, 0.000, 1.000)
		"ecce87",	// Wheat (0.925, 0.808, 0.529)
		"f7a58b",	// Peach (0.969, 0.647, 0.545)
		"bfff81",	// Mint (0.749, 1.000, 0.506)
		"dbb8eb",	// Lavender (0.859, 0.722, 0.922)
		"4f5054",	// Coal (0.310, 0.314, 0.333)
		"ecf0ff",	// Snow (0.925, 0.941, 1.000)
		"00781e",	// Emerald (0.000, 0.471, 0.118)
		"a56e34",	// Peanut (0.647, 0.435, 0.204)
		// Unused (Included for completeness)
		/*
		"2e2d2e",	// Black (0.180, 0.176, 0.180)
		"2e2d2e",	// Black (0.180, 0.176, 0.180)
		"2e2d2e",	// Black (0.180, 0.176, 0.180)
		"2e2d2e"	// Black (0.180, 0.176, 0.180)
		*/
	};

	// Player Properties tab

	for (int i = 0; i < 24; i++) {
		
		PlayerRow row;

		auto* number = new QLabel(QString::number(i + 1));
		number->setAlignment(Qt::AlignCenter);

		row.controller = new QComboBox;
		row.controller->addItems({ "None", "User", "Computer", "Neutral", "Rescuable" });

		std::string name = "Player " + std::to_string(i+1);
		row.name = new QLineEdit(QString::fromStdString(name));
		row.name->setEnabled(false);

		row.color = new QLabel;
		row.color->setStyleSheet(QString("background-color: #%1; margin: 2px 0px;").arg(player_colors[i]));

		row.race = new QComboBox;
		row.race->addItems({ "Human", "Orc", "Undead", "Night Elf", "Selectable" });
		row.race->setEnabled(false);
		row.race->setCurrentIndex(i%4);

		row.fixed_start_position = new QCheckBox;
		row.fixed_start_position->setCheckState(Qt::CheckState::Unchecked);
		row.fixed_start_position->setEnabled(false);

		ui.playerGrid->addWidget(number, i+1, 0);
		ui.playerGrid->addWidget(row.controller, i+1, 1);
		ui.playerGrid->addWidget(row.name, i+1, 2);
		ui.playerGrid->addWidget(row.color, i+1, 3);
		ui.playerGrid->addWidget(row.race, i+1, 4);
		ui.playerGrid->addWidget(row.fixed_start_position, i+1, 5, Qt::AlignHCenter);

		player_rows.push_back(row);
	}

	ui.playerGrid->setVerticalSpacing(0);

	for (const auto& player: map->info.players) {
		
		switch (player.type) {
		case PlayerType::human:
			player_rows[player.internal_number].controller->setCurrentIndex(1);
			break;
		case PlayerType::computer:
			player_rows[player.internal_number].controller->setCurrentIndex(2);
			break;
		case PlayerType::neutral:
			player_rows[player.internal_number].controller->setCurrentIndex(3);
			break;
		case PlayerType::rescuable:
			player_rows[player.internal_number].controller->setCurrentIndex(4);
			break;
		}

		player_rows[player.internal_number].name->setText(QString::fromUtf8(map->trigger_strings.string(player.name)));
		player_rows[player.internal_number].name->setEnabled(true);

		switch (player.race) {
		case PlayerRace::human:
			player_rows[player.internal_number].race->setCurrentIndex(0);
			break;
		case PlayerRace::orc:
			player_rows[player.internal_number].race->setCurrentIndex(1);
			break;
		case PlayerRace::undead:
			player_rows[player.internal_number].race->setCurrentIndex(2);
			break;
		case PlayerRace::night_elf:
			player_rows[player.internal_number].race->setCurrentIndex(3);
			break;
		case PlayerRace::selectable:
			player_rows[player.internal_number].race->setCurrentIndex(4);
			break;
		}
		player_rows[player.internal_number].race->setEnabled(true);

		if (player.fixed_start_position == 0) {
			player_rows[player.internal_number].fixed_start_position->setCheckState(Qt::CheckState::Unchecked);
		} else {
			player_rows[player.internal_number].fixed_start_position->setCheckState(Qt::CheckState::Checked);
		}
		player_rows[player.internal_number].fixed_start_position->setEnabled(true);
	}

	connect(ui.buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button) {
		if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole) {
			restoreDefaults();
		}
	});
	
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

	for (int player_slot = 0; player_slot < 24; player_slot++) {
		connect(player_rows[player_slot].controller, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, player_slot](int controllerTypeIndex) {
			updateController(player_slot, controllerTypeIndex);
		});
	}

    show();
}


bool ScenarioInfoEditor::save() const {
	for (size_t slot = 0; slot < 24; slot++) {
		const int controller_type = player_rows[slot].controller->currentIndex();

		int found_index = -1;

		for (int i = 0; i < map->info.players.size(); i++) {
			if (map->info.players[i].internal_number == slot) {
				found_index = i;
				break;
			}
		}

		// Controller == None ?
		if (controller_type == 0) {
			if (found_index != -1) {
				map->info.players.erase(map->info.players.begin() + found_index);
			}
			continue;
		}

		// Controller is not None and we didn't an existing player
		// Creating a new player data
		if (found_index == -1) {
			auto& new_player = map->info.players.emplace_back();
			
			new_player.internal_number = slot;
			new_player.type = PlayerType::human;
			new_player.race = PlayerRace::human;
			new_player.fixed_start_position = 0;
			new_player.name = "";
			new_player.starting_position = { 0.f, 0.f };
			new_player.ally_low_priorities_flags = 0;
			new_player.ally_high_priorities_flags = 0;
			new_player.enemy_low_priorities_flags = 0;
			new_player.enemy_high_priorities_flags = 0;

			found_index = map->info.players.size()-1;
		}

		auto& p = map->info.players[found_index];

		switch (controller_type) {
		case 1:
			p.type = PlayerType::human;
			break;
		case 2:
			p.type = PlayerType::computer;
			break;
		case 3:
			p.type = PlayerType::neutral;
			break;
		case 4:
			p.type = PlayerType::rescuable;
			break;
		}

		map->trigger_strings.set_string(p.name, player_rows[slot].name->text().toStdString());

		switch (player_rows[slot].race->currentIndex()) {
		case 0:
			p.race = PlayerRace::human;
			break;
		case 1:
			p.race = PlayerRace::orc;
			break;
		case 2:
			p.race = PlayerRace::undead;
			break;
		case 3:
			p.race = PlayerRace::night_elf;
			break;
		case 4:
			p.race = PlayerRace::selectable;
			break;
		}
		
		p.fixed_start_position = (player_rows[p.internal_number].fixed_start_position->checkState() == Qt::CheckState::Checked) ? 1 : 0;
	}

	return true;
}


void ScenarioInfoEditor::updateController(int slotIndex, int controllerTypeIndex) {
	// Is controller type NOT Human ?
	if (controllerTypeIndex != 1) {
		// Is there any other Human controller type ?
		bool found_human = false;
		for (const auto& player : player_rows) {
			if (player.controller->currentIndex() == 1) {
				found_human = true;
				break;
			}
		}

		if (found_human == false) {
			player_rows[slotIndex].controller->setCurrentIndex(1);
			player_rows[slotIndex].name->setEnabled(true);
			player_rows[slotIndex].race->setEnabled(true);
			player_rows[slotIndex].fixed_start_position->setEnabled(true);
			QMessageBox::information(this, "ScenarioInfoEditor error", "At least one player must be human controlled.");
			return;
		}
	}

	// Is controller type None ?
	if (controllerTypeIndex == 0) {
		player_rows[slotIndex].name->setEnabled(false);
		player_rows[slotIndex].race->setEnabled(false);
		player_rows[slotIndex].fixed_start_position->setEnabled(false);
	} else {
		player_rows[slotIndex].name->setEnabled(true);
		player_rows[slotIndex].race->setEnabled(true);
		player_rows[slotIndex].fixed_start_position->setEnabled(true);
	}
}

void ScenarioInfoEditor::restoreDefaults() {
	int choice = QMessageBox::question(
		this,
		"Do you want to restore defaults?",
		"Are you sure you want to restore default values?",
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);

	if (choice == QMessageBox::No) {
		return;
	}

	switch (ui.tabs->currentIndex()) {
	case 0:
		restorePlayerProperties();
		break;
	}
}

void ScenarioInfoEditor::restorePlayerProperties() {
	// Restoring Player 1 defaults
	player_rows[0].controller->setCurrentIndex(1);

	player_rows[0].name->setText("Player 1");
	player_rows[0].name->setEnabled(true);

	player_rows[0].race->setCurrentIndex(0);
	player_rows[0].race->setEnabled(true);

	player_rows[0].fixed_start_position->setCheckState(Qt::CheckState::Unchecked);
	player_rows[0].fixed_start_position->setEnabled(true);

	// Restoring Player 2 - 24 defaults
	for (int i = 1; i < 24; i++) {
		player_rows[i].controller->setCurrentIndex(0);

		std::string name = "Player " + std::to_string(i+1);
		player_rows[i].name->setText(QString::fromStdString(name));
		player_rows[i].name->setEnabled(false);

		player_rows[i].race->setCurrentIndex(i%4);
		player_rows[i].race->setEnabled(false);

		player_rows[i].fixed_start_position->setCheckState(Qt::CheckState::Unchecked);
		player_rows[i].fixed_start_position->setEnabled(false);
	}
}