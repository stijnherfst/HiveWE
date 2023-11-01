module;

#include <string>
#include <vector>
#include <iostream>

#include <glm/glm.hpp>

export module MapInfo;

import BinaryReader;
import BinaryWriter;
import Hierarchy;
import Utilities;

export enum class PlayerType {
	human,
	computer,
	neutral,
	rescuable
};

export enum class PlayerRace {
	selectable,
	human,
	orc,
	undead,
	night_elf
};

struct PlayerData {
	int internal_number;
	PlayerType type;
	PlayerRace race;
	int fixed_start_position;
	std::string name;
	glm::vec2 starting_position;
	uint32_t ally_low_priorities_flags;
	uint32_t ally_high_priorities_flags;
	uint32_t enemy_low_priorities_flags = 0;
	uint32_t enemy_high_priorities_flags = 0;
};

struct ForceData {
	bool allied;
	bool allied_victory;
	bool share_vision;
	bool share_unit_control;
	bool share_advanced_unit_control;
	int player_masks;
	std::string name;
};

struct UpgradeAvailability {
	int player_flags;
	std::string id;
	int level;
	int availability;
};

struct TechAvailability {
	int player_flags;
	std::string id;
};

struct RandomUnitLine {
	int chance;
	std::vector<std::string> ids;
};

struct RandomUnitTable {
	int creation_number;
	std::string name;
	std::vector<int> positions;
	std::vector<RandomUnitLine> lines;
};

struct RandomItemTable {
	int creation_number;
	std::string name;
	std::vector<ItemSet> item_sets;
};

export class MapInfo {
  public:
	int map_version;
	int editor_version;
	int game_version_major;
	int game_version_minor;
	int game_version_patch;
	int game_version_build;
	std::string name;
	std::string author;
	std::string description;
	std::string suggested_players;

	glm::vec2 camera_left_bottom;
	glm::vec2 camera_right_top;
	glm::vec2 camera_left_top;
	glm::vec2 camera_right_bottom;

	glm::ivec4 camera_complements;

	int playable_width;
	int playable_height;

	int all_flags;
	bool hide_minimap_preview;
	bool modif_ally_priorities;
	bool melee_map;
	bool unknown;
	bool masked_area_partially_visible;
	bool fixed_player_settings;
	bool custom_forces;
	bool custom_techtree;
	bool custom_abilities;
	bool custom_upgrades;
	bool unknown2;
	bool cliff_shore_waves;
	bool rolling_shore_waves;
	bool unknown3;
	bool unknown4;
	bool item_classification;
	bool water_tinting;
	bool accurate_probability_for_calculations;
	bool custom_ability_skins;

	int loading_screen_number;
	std::string loading_screen_model;
	std::string loading_screen_text;
	std::string loading_screen_title;
	std::string loading_screen_subtitle;

	int game_data_set;

	std::string prologue_screen_model;
	std::string prologue_text;
	std::string prologue_title;
	std::string prologue_subtitle;

	int fog_style;
	float fog_start_z_height;
	float fog_end_z_height;
	float fog_density;
	glm::u8vec4 fog_color;

	int weather_id;
	std::string custom_sound_environment;
	char custom_light_tileset;
	glm::u8vec4 water_color;

	bool lua;
	uint32_t supported_modes;
	uint32_t game_data_version;

	std::vector<PlayerData> players;
	std::vector<ForceData> forces;
	std::vector<UpgradeAvailability> available_upgrades;
	std::vector<TechAvailability> available_tech;
	std::vector<RandomUnitTable> random_unit_tables;
	std::vector<RandomItemTable> random_item_tables;

	static constexpr int write_version = 31;
	static constexpr int write_editor_version = 6105;
	static constexpr int write_game_version_major = 1;
	static constexpr int write_game_version_minor = 32;
	static constexpr int write_game_version_patch = 1;
	static constexpr int write_game_version_build = 14604;

	void load() {
		BinaryReader reader = hierarchy.map_file_read("war3map.w3i");

		const int version = reader.read<uint32_t>();

		if (version != 31 && version != 28 && version != 25 && version != 18 && version != 15) {
			std::cout << "Unknown war3map.w3i version\n";
		}

		if (version >= 18) {
			map_version = reader.read<uint32_t>();
			editor_version = reader.read<uint32_t>();

			if (version >= 28) {
				game_version_major = reader.read<uint32_t>();
				game_version_minor = reader.read<uint32_t>();
				game_version_patch = reader.read<uint32_t>();
				game_version_build = reader.read<uint32_t>();
			}
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
		unknown = flags & 0x0008; // playable map size was large
		masked_area_partially_visible = flags & 0x0010;
		fixed_player_settings = flags & 0x0020;
		custom_forces = flags & 0x0040;
		custom_techtree = flags & 0x0080;
		custom_abilities = flags & 0x0100;
		custom_upgrades = flags & 0x0200;
		unknown2 = flags & 0x0400; // has properties menu been opened
		cliff_shore_waves = flags & 0x0800;
		rolling_shore_waves = flags & 0x1000;
		unknown3 = flags & 0x2000; // has terrain fog
		unknown4 = flags & 0x4000; // requires expansion
		item_classification = flags & 0x8000;
		water_tinting = flags & 0x10000;
		accurate_probability_for_calculations = flags & 0x20000;
		custom_ability_skins = flags & 0x40000;

		// Tileset
		reader.advance(1);

		if (version >= 25) { // TFT
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

			if (version >= 28) {
				lua = reader.read<uint32_t>() == 1;
			}

			if (version >= 31) {
				supported_modes = reader.read<uint32_t>();
				game_data_version = reader.read<uint32_t>();
			}
		} else if (version == 18) { // RoC
			loading_screen_number = reader.read<uint32_t>();
			loading_screen_text = reader.read_c_string();
			loading_screen_title = reader.read_c_string();
			loading_screen_subtitle = reader.read_c_string();

			// game_data_set = reader.read<uint32_t>();
			reader.advance(4); // ToDo RoC map loading screen number

			prologue_text = reader.read_c_string();
			prologue_title = reader.read_c_string();
			prologue_subtitle = reader.read_c_string();
		} else {
			reader.advance(1); // unknown, loading screen number but only 1 digit?
			loading_screen_text = reader.read_c_string();
			loading_screen_title = reader.read_c_string();
			loading_screen_subtitle = reader.read_c_string();
			reader.advance(4); // prologue stuff?
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
			if (version >= 31) {
				i.enemy_low_priorities_flags = reader.read<uint32_t>();
				i.enemy_high_priorities_flags = reader.read<uint32_t>();
			}
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

		if (reader.remaining() < 4) {
			return;
		}

		available_upgrades.resize(reader.read<uint32_t>());
		for (auto&& i : available_upgrades) {
			i.player_flags = reader.read<uint32_t>();
			i.id = reader.read_string(4);
			i.level = reader.read<uint32_t>();
			i.availability = reader.read<uint32_t>();
		}

		if (reader.remaining() < 4) {
			return;
		}

		available_tech.resize(reader.read<uint32_t>());
		for (auto&& i : available_tech) {
			i.player_flags = reader.read<uint32_t>();
			i.id = reader.read_string(4);
		}

		if (reader.remaining() < 4) {
			return;
		}

		random_unit_tables.resize(reader.read<uint32_t>());
		for (auto&& i : random_unit_tables) {
			i.creation_number = reader.read<uint32_t>();
			i.name = reader.read_c_string();
			i.positions = reader.read_vector<int>(reader.read<uint32_t>());

			i.lines.resize(reader.read<uint32_t>());
			for (auto&& j : i.lines) {
				j.chance = reader.read<uint32_t>();
				for (size_t k = 0; k < i.positions.size(); k++) {
					j.ids.push_back(reader.read_string(4));
				}
			}
		}

		if (reader.remaining() < 4) {
			return;
		}

		if (version >= 25) {
			random_item_tables.resize(reader.read<uint32_t>());
			for (auto&& i : random_item_tables) {
				i.creation_number = reader.read<uint32_t>();
				i.name = reader.read_c_string();
				i.item_sets.resize(reader.read<uint32_t>());
				for (auto&& j : i.item_sets) {
					j.items.resize(reader.read<uint32_t>());
					for (auto&& [chance, id] : j.items) {
						chance = reader.read<uint32_t>();
						id = reader.read_string(4);
					}
				}
			}
		}
	}

	void save(char tileset) const {
		BinaryWriter writer;

		writer.write(write_version);
		writer.write(map_version);
		writer.write(write_editor_version);
		writer.write(write_game_version_major);
		writer.write(write_game_version_minor);
		writer.write(write_game_version_patch);
		writer.write(write_game_version_build);
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

		const int flags = hide_minimap_preview * 0x0001 | modif_ally_priorities * 0x0002 | melee_map * 0x0004 | unknown * 0x0008 | masked_area_partially_visible * 0x0010 | fixed_player_settings * 0x0020 | custom_forces * 0x0040 | custom_techtree * 0x0080 | custom_abilities * 0x0100 | custom_upgrades * 0x0200 | unknown2 * 0x0400 | cliff_shore_waves * 0x0800 | rolling_shore_waves * 0x1000 | unknown3 * 0x2000 | unknown4 * 0x4000 | item_classification * 0x8000 | water_tinting * 0x10000 | accurate_probability_for_calculations * 0x20000 | custom_ability_skins * 0x40000;

		writer.write(flags);

		writer.write(tileset);

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

		writer.write(supported_modes);
		writer.write(game_data_version);

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
			writer.write(i.enemy_low_priorities_flags);
			writer.write(i.enemy_high_priorities_flags);
		}

		writer.write<uint32_t>(forces.size());
		for (const auto& i : forces) {
			const uint32_t force_flags = i.allied * 0b00000001 | i.allied_victory * 0b00000010 | i.share_vision * 0b00000100 | i.share_unit_control * 0b00010000 | i.share_advanced_unit_control * 0b00100000;
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
			writer.write(i.creation_number);
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
			writer.write(i.creation_number);
			writer.write_c_string(i.name);

			writer.write<uint32_t>(i.item_sets.size());
			for (const auto& j : i.item_sets) {
				writer.write<uint32_t>(j.items.size());
				for (const auto& [chance, id] : j.items) {
					writer.write(chance);
					writer.write_string(id);
				}
			}
		}

		hierarchy.map_file_write("war3map.w3i", writer.buffer);
	}
};