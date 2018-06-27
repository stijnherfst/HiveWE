#pragma once

enum class PlayerType {
	human,
	computer,
	neutral,
	rescuable
};

enum class PlayerRace {
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
	int ally_low_priorities_flags;
	int ally_high_priorities_flags;
};

enum class FocusFlags {
	allied,
	allied_victory,
	share_vision,
	share_unit_control,
	share_advanced_unit_control
};

struct ForceData {
	FocusFlags focus_flags;
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
	int number;
	std::string name;
	std::vector<int> positions;
	std::vector<RandomUnitLine> lines;
};

struct RandomItemSets {
	std::vector<std::tuple<int, std::string>> items;
};

struct RandomItemTable {
	int number;
	std::string name;
	std::vector<RandomItemSets> item_sets;
};

class MapInfo {
public:
	int map_version;
	int editor_version;
	std::string name;
	std::string author;
	std::string description;
	std::string suggested_players;

	glm::vec2 camera_top_left;
	glm::vec2 camera_top_right;
	glm::vec2 camera_bottom_left;
	glm::vec2 camera_bottom_right;

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

	std::vector<PlayerData> players;
	std::vector<ForceData> forces;
	std::vector<UpgradeAvailability> available_upgrades;
	std::vector<TechAvailability> available_tech;
	std::vector<RandomUnitTable> random_unit_tables;
	std::vector<RandomItemTable> random_item_tables;

	static constexpr int write_version = 25;

	void load(BinaryReader& reader);
	void save() const;
};