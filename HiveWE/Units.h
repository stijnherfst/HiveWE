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
	std::shared_ptr<StaticMesh> mesh;

	void update();
};

class Units {
	std::unordered_map<std::string, std::shared_ptr<StaticMesh>> id_to_mesh;

	static constexpr int write_version = 8;
	static constexpr int write_subversion = 11;
public:
	std::vector<Unit> units;
	std::vector<Unit> items;
	QuadTree<Unit> tree; // ToDo remove

	bool load(BinaryReader& reader, Terrain& terrain);
	void save() const;
	void load_unit_modifications(BinaryReader& reader);
	void load_item_modifications(BinaryReader& reader);
	void update_area(const QRect& area);
	void create();
	void render() const;

	std::shared_ptr<StaticMesh> get_mesh(const std::string& id);
};