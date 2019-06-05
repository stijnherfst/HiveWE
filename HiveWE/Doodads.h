#pragma once


struct Doodad {
	static int auto_increment;
	std::string id = "";
	int variation = 0;
	glm::vec3 position = {0, 0, 0};
	glm::vec3 scale = {0, 0, 0};
	float angle = 0.f;

	enum class State {
		invisible_non_solid,
		visible_non_solid,
		visible_solid
	};
	State state = State::visible_solid;
	int life = 100;

	int item_table_pointer = -1;
	std::vector<ItemSet> item_sets;

	int creation_number;

	// Auxilirary data
	glm::mat4 matrix = glm::mat4(1.f);
	std::shared_ptr<StaticMesh> mesh;
	std::shared_ptr<Texture> pathing;

	Doodad() {
		creation_number = ++auto_increment;
	}

	void update();
};

struct SpecialDoodad {
	std::string id;
	int variation;
	glm::vec3 position;
	glm::mat4 matrix = glm::mat4(1.f);
	std::shared_ptr<StaticMesh> mesh;
};

class Doodads {
	std::vector<SpecialDoodad> special_doodads;
	
	std::unordered_map<std::string, std::shared_ptr<StaticMesh>> id_to_mesh;

	std::shared_ptr<Shader> shader;

	static constexpr int write_version = 8;
	static constexpr int write_subversion = 11;
	static constexpr int write_special_version = 0;
public:
	std::vector<Doodad> doodads;

	bool load(BinaryReader& reader, Terrain& terrain);
	void save() const;
	void load_destructible_modifications(BinaryReader& reader);
	void load_doodad_modifications(BinaryReader& reader);
	//void update_area(const QRect& area);
	void create();
	void render();

	Doodad& add_doodad(std::string id, int variation, glm::vec3 position);
	Doodad& add_doodad(Doodad doodad);

	void remove_doodad(Doodad* doodad);

	std::vector<Doodad*> query_area(const QRectF& area);
	void remove_doodads(const std::vector<Doodad*>& list);

	void update_doodad_pathing(const QRectF& area);

	std::shared_ptr<StaticMesh> get_mesh(std::string id, int variation);
};

// Undo/redo structures
class DoodadAddAction : public TerrainUndoAction {
public:
	std::vector<Doodad> doodads;

	void undo() override;
	void redo() override;
};

class DoodadDeleteAction : public TerrainUndoAction {
public:
	std::vector<Doodad> doodads;

	void undo() override;
	void redo() override;
};

class DoodadStateAction : public TerrainUndoAction {
public:
	std::vector<Doodad> old_doodads;
	std::vector<Doodad> new_doodads;

	void undo() override;
	void redo() override;
};