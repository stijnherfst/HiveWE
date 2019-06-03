#include "stdafx.h"

void MapInfo::load(BinaryReader& reader) {
	const int version = reader.read<uint32_t>();

	if (version != 18 && version != 25 && version != 28) {
		std::cout << "Unknown war3map.w3i version\n";
	}

	map_version = reader.read<uint32_t>();
	editor_version = reader.read<uint32_t>();
	if (version == 28) {
		game_version_major = reader.read<uint32_t>();
		game_version_minor = reader.read<uint32_t>();
		game_version_patch = reader.read<uint32_t>();
		game_version_build = reader.read<uint32_t>();
	}
	name = reader.read_c_string();
	author = reader.read_c_string();
	description = reader.read_c_string();
	suggested_players = reader.read_c_string();

	camera_left_bottom = reader.read<glm::vec2>();
	camera_right_top = reader.read<glm::vec2>();
	camera_left_top = reader.read<glm::vec2>();
	camera_right_bottom = reader.read<glm::vec2>();

	camera_complements = reader.read<glm::ivec4>();

	playable_width = reader.read<uint32_t>();
	playable_height = reader.read<uint32_t>();

	const int flags = reader.read<uint32_t>();
	hide_minimap_preview = flags & 0x0001;
	modif_ally_priorities = flags & 0x0002;
	melee_map = flags & 0x0004;
	unknown = flags & 0x0008;
	masked_area_partially_visible = flags & 0x0010;
	fixed_player_settings = flags & 0x0020;
	custom_forces = flags & 0x0040;
	custom_techtree = flags & 0x0080;
	custom_abilities = flags & 0x0100;
	custom_upgrades = flags & 0x0200;
	unknown2 = flags & 0x0400;
	cliff_shore_waves = flags & 0x0800;
	rolling_shore_waves = flags & 0x1000;
	unknown3 = flags & 0x2000;
	unknown4 = flags & 0x4000;
	item_classification = flags & 0x8000;
	water_tinting = flags & 0x10000;

	// Tileset
	reader.advance(1);

	if (version == 25 || version == 28) { // TFT
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

		fog_style = reader.read<uint32_t>();
		fog_start_z_height = reader.read<float>();
		fog_end_z_height = reader.read<float>();
		fog_density = reader.read<float>();
		fog_color = reader.read<glm::u8vec4>();

		weather_id = reader.read<uint32_t>();
		custom_sound_environment = reader.read_c_string();
		custom_light_tileset = reader.read<uint8_t>();
		water_color = reader.read<glm::u8vec4>();

		if (version == 28) {
			lua = reader.read<uint32_t>() == 1;
		}
	} else if (version == 18) { // RoC
		loading_screen_number = reader.read<uint32_t>();
		loading_screen_text = reader.read_c_string();
		loading_screen_title = reader.read_c_string();
		loading_screen_subtitle = reader.read_c_string();

		//game_data_set = reader.read<uint32_t>();
		reader.advance(4); // ToDo RoC map loading screen number

		prologue_text = reader.read_c_string();
		prologue_title = reader.read_c_string();
		prologue_subtitle = reader.read_c_string();
	}
	players.resize(reader.read<uint32_t>());
	for (auto&& i : players) {
		i.internal_number = reader.read<uint32_t>();
		i.type = static_cast<PlayerType>(reader.read<uint32_t>() - 1);
		i.race = static_cast<PlayerRace>(reader.read<uint32_t>());
		i.fixed_start_position = reader.read<uint32_t>();
		i.name = reader.read_c_string();
		i.starting_position = reader.read<glm::vec2>();
		i.ally_low_priorities_flags = reader.read<uint32_t>();
		i.ally_high_priorities_flags = reader.read<uint32_t>();
	}

	forces.resize(reader.read<uint32_t>());
	for (auto&& i : forces) {
		const uint32_t force_flags = reader.read<uint32_t>();
		i.allied = force_flags & 0b00000001;
		i.allied_victory = force_flags & 0b00000010;
		i.share_vision = force_flags & 0b00001000;
		i.share_unit_control = force_flags & 0b00010000;
		i.share_advanced_unit_control = force_flags & 0b00100000;

		i.player_masks = reader.read<uint32_t>();
		i.name = reader.read_c_string();
	}

	// Oftentimes when maps are protected file is cut short here with just 1 byte left instead of at least 12
	if (map->is_protected) {
		return;
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

	random_unit_tables.resize(reader.read<uint32_t>());
	for (auto&& i : random_unit_tables) {
		i.number = reader.read<uint32_t>();
		i.name = reader.read_c_string();
		i.positions = reader.read_vector<int>(reader.read<uint32_t>());

		i.lines.resize(reader.read<uint32_t>());
		for (auto&& j : i.lines) {
			j.chance = reader.read<uint32_t>();
			for (int k = 0; k < i.positions.size(); k++) {
				j.ids.push_back(reader.read_string(4));
			}
		}
	}

	if (version == 25 || version == 28) {
		random_item_tables.resize(reader.read<uint32_t>());
		for (auto&& i : random_item_tables) {
			i.number = reader.read<uint32_t>();
			i.name = reader.read_c_string();
			i.item_sets.resize(reader.read<uint32_t>());
			for (auto&& j : i.item_sets) {
				j.items.resize(reader.read<uint32_t>());
				for (auto&&[chance, id] : j.items) {
					chance = reader.read<uint32_t>();
					id = reader.read_string(4);
				}
			}
		}
	}
}

void MapInfo::save() const {
	BinaryWriter writer;

	writer.write(write_version);
	writer.write(map_version);
	writer.write(editor_version);
	writer.write(game_version_major);
	writer.write(game_version_minor);
	writer.write(game_version_patch);
	writer.write(game_version_build);
	writer.write_c_string(name);
	writer.write_c_string(author);
	writer.write_c_string(description);
	writer.write_c_string(suggested_players);

	writer.write(camera_left_bottom);
	writer.write(camera_right_top);
	writer.write(camera_left_top);
	writer.write(camera_right_bottom);

	writer.write(camera_complements);

	writer.write(playable_width);
	writer.write(playable_height);

	const int flags = hide_minimap_preview * 0x0001
		| modif_ally_priorities * 0x0002
		| melee_map * 0x0004
		| unknown * 0x0008
		| masked_area_partially_visible * 0x0010
		| fixed_player_settings * 0x0020
		| custom_forces * 0x0040
		| custom_techtree * 0x0080
		| custom_abilities * 0x0100
		| custom_upgrades * 0x0200
		| unknown2 * 0x0400
		| cliff_shore_waves * 0x0800
		| rolling_shore_waves * 0x1000
		| unknown3 * 0x2000
		| unknown4 * 0x4000
		| item_classification * 0x8000
		| water_tinting * 0x10000;
	writer.write(flags);

	writer.write(map->terrain.tileset);

	writer.write(loading_screen_number);
	writer.write_c_string(loading_screen_model);
	writer.write_c_string(loading_screen_text);
	writer.write_c_string(loading_screen_title);
	writer.write_c_string(loading_screen_subtitle);

	writer.write(game_data_set);

	writer.write_c_string(prologue_screen_model);
	writer.write_c_string(prologue_text);
	writer.write_c_string(prologue_title);
	writer.write_c_string(prologue_subtitle);

	writer.write(fog_style);
	writer.write(fog_start_z_height);
	writer.write(fog_end_z_height);
	writer.write(fog_density);
	writer.write(fog_color);

	writer.write(weather_id);
	writer.write_c_string(custom_sound_environment);
	writer.write(custom_light_tileset);
	writer.write(water_color);
	
	writer.write((uint32_t)lua);

	writer.write<uint32_t>(players.size());
	for (const auto& i : players) {
		writer.write(i.internal_number);
		writer.write(static_cast<int>(i.type) + 1);
		writer.write(static_cast<int>(i.race));
		writer.write(i.fixed_start_position);
		writer.write_c_string(i.name);
		writer.write(i.starting_position);
		writer.write(i.ally_low_priorities_flags);
		writer.write(i.ally_high_priorities_flags);
	}

	writer.write<uint32_t>(forces.size());
	for (const auto& i : forces) {
		const uint32_t force_flags = i.allied * 0b00000001
			| i.allied_victory * 0b00000010
			| i.share_vision * 0b00000100
			| i.share_unit_control * 0b00010000
			| i.share_advanced_unit_control * 0b00100000;
		writer.write(force_flags);

		writer.write(i.player_masks);
		writer.write_c_string(i.name);
	}

	writer.write<uint32_t>(available_upgrades.size());
	for (const auto& i : available_upgrades) {
		writer.write(i.player_flags);
		writer.write_string(i.id);
		writer.write(i.level);
		writer.write(i.availability);
	}

	writer.write<uint32_t>(available_tech.size());
	for (const auto& i : available_tech) {
		writer.write(i.player_flags);
		writer.write_string(i.id);
	}

	writer.write<uint32_t>(random_unit_tables.size());
	for (const auto& i : random_unit_tables) {
		writer.write(i.number);
		writer.write_c_string(i.name);
		writer.write_vector(i.positions);

		writer.write<uint32_t>(i.lines.size());
		for (const auto& j : i.lines) {
			writer.write(j.chance);
			writer.write_vector(j.ids);
		}
	}

	writer.write<uint32_t>(random_item_tables.size());
	for (const auto& i : random_item_tables) {
		writer.write(i.number);
		writer.write_c_string(i.name);

		writer.write<uint32_t>(i.item_sets.size());
		for (const auto& j : i.item_sets) {
			writer.write<uint32_t>(j.items.size());
			for (const auto&[chance, id] : j.items) {
				writer.write(chance);
				writer.write_string(id);
			}
		}
	}

	hierarchy.map_file_write("war3map.w3i", writer.buffer);
}