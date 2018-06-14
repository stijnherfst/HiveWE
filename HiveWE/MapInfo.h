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

class MapInfo {
public:
	int map_version;
	int editor_version;
	std::string name;
	std::string author;
	std::string description;
	std::string suggested_players;
	int playable_width;
	int playable_height;
	int flags;
	char tileset;


	int loading_screen_number;
	fs::path loading_screen_model;
	std::string loading_screen_text;
	std::string loading_screen_title;
	std::string loading_screen_subtitle;

	int game_data_set;

	fs::path prologue_screen_model;
	std::string prologue_text;
	std::string prologue_title;
	std::string prologue_subtitle;

	int uses_fog;
	float fog_start_z_height;
	float fog_end_z_height;
	float fog_density;
	glm::vec4 fog_color;

	int weather_id;
	std::string custom_sound_environment;
	char custom_light_tileset;
	glm::vec4 water_color;

	std::vector<PlayerData> players;
	std::vector<ForceData> forces;
	std::vector<UpgradeAvailability> available_upgrades;
	std::vector<TechAvailability> available_tech;

	static constexpr int write_version = 25;


	void load(BinaryReader& reader);
	void save() const;
};