#include "stdafx.h"

void MapInfo::load(BinaryReader& reader) {
	int version = reader.read<uint32_t>();

	map_version = reader.read<uint32_t>();
	editor_version = reader.read<uint32_t>();
	name = reader.read_c_string();
	author = reader.read_c_string();
	description = reader.read_c_string();
	suggested_players = reader.read_c_string();

	// camera bounds
	reader.advance(48);

	playable_width = reader.read<uint32_t>();
	playable_height = reader.read<uint32_t>();
	flags = reader.read<uint32_t>();
	tileset = reader.read<uint8_t>();

	if (version == 25) { // TFT
		loading_screen_number = reader.read<uint32_t>();
		loading_screen_model = reader.read_c_string();
		loading_screen_text = reader.read_c_string();
		loading_screen_title = reader.read_c_string();
		loading_screen_subtitle = reader.read_c_string();

		game_data_set = reader.read<uint32_t>();

		prologue_screen_model = reader.read_c_string();
		prologue_text = reader.read_c_string();
		prologue_title = reader.read_c_string();
		prologue_subtitle = reader.read_c_string();

		uses_fog = reader.read<uint32_t>();
		fog_start_z_height = reader.read<float>();
		fog_end_z_height = reader.read<float>();
		fog_density = reader.read<float>();
		fog_color = reader.read<glm::u8vec4>();

		weather_id = reader.read<uint32_t>();
		custom_sound_environment = reader.read_c_string();
		custom_light_tileset = reader.read<uint8_t>();
		water_color = reader.read<glm::u8vec4>();

		players.resize(reader.read<uint32_t>());
		for (auto&& i : players) {
			i.internal_number = reader.read<uint32_t>();
			i.type = static_cast<PlayerType>(reader.read<uint32_t>());
			i.race = static_cast<PlayerRace>(reader.read<uint32_t>());
			i.fixed_start_position = reader.read<uint32_t>();
			i.name = reader.read_c_string();
			i.starting_position = reader.read<glm::vec2>();
			i.ally_low_priorities_flags = reader.read<uint32_t>();
			i.ally_high_priorities_flags = reader.read<uint32_t>();
		}

		forces.resize(reader.read<uint32_t>());
		for (auto&& i : forces) {
			i.focus_flags = static_cast<FocusFlags>(reader.read<uint32_t>());
			i.player_masks = reader.read<uint32_t>();
			i.name = reader.read_c_string();
		}

		available_upgrades.resize(reader.read<uint32_t>());
		for (auto&& i : available_upgrades) {
			i.player_flags = reader.read<uint32_t>();
			i.id = reader.read_string(4);
			i.level = reader.read<uint32_t>();
			i.availability = reader.read<uint32_t>();
		}

		available_tech.resize(reader.read<uint32_t>());
		for (auto&& i : available_tech) {
			i.player_flags = reader.read<uint32_t>();
			i.id = reader.read_string(4);
		}


	} else if (version == 18) { // RoC

	}

}
void MapInfo::save() const {

}