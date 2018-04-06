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
};

class Units {
	slk::SLK units_slk;
	slk::SLK units_meta_slk;
	slk::SLK items_slk;

	std::vector<Unit> units;

	std::unordered_map<std::string, std::shared_ptr<StaticMesh>> id_to_mesh;
public:
	bool load(BinaryReader& reader, Terrain& terrain);
	void load_unit_modifications(BinaryReader& reader);
	void load_item_modifications(BinaryReader& reader);
	void create();
	void render();
};
