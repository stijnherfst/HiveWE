#pragma once

struct Unit {
	std::string id;
	int variation;
	glm::vec3 position;
	float angle;
	glm::vec3 scale;

	uint8_t flags;
	int player;

	uint8_t unknown1;
	uint8_t unknown2;
	int health;
	int mana;

	int item_table_pointer;
	std::vector<ItemSet> item_sets;

	int gold;
	float target_acquisition;

	int level;
	int strength;
	int agility;
	int intelligence;

	// Slot, ID
	std::vector<std::pair<uint32_t, std::string>> items;

	// ID, autocast, ability level
	std::vector<std::tuple<std::string, uint32_t, uint32_t>> abilities;

	int random_type;
	std::vector<uint8_t> random;

	int custom_color;
	int waygate;
	int creation_number;

	glm::mat4 matrix = glm::mat4(1.f);
};

class Units {

	std::vector<Unit> units;

	std::unordered_map<std::string, std::shared_ptr<StaticMesh>> id_to_mesh;

	static constexpr int write_version = 8;
	static constexpr int write_subversion = 11;
public:
	slk::SLK units_slk;
	slk::SLK units_meta_slk;
	slk::SLK items_slk;

	QuadTree<Unit> tree;

	bool load(BinaryReader& reader, Terrain& terrain);
	void save() const;
	void load_unit_modifications(BinaryReader& reader);
	void load_item_modifications(BinaryReader& reader);
	void update_area(const QRect& area);
	void create();
	void render();
};