#pragma once

enum class DoodadState {
	invisible_non_solid,
	visible_non_solid,
	visible_solid
};

struct Doodad {
	std::string id;
	int variation;
	glm::vec3 position;
	glm::vec3 scale;
	float angle;
	DoodadState state;
	uint8_t life;

	glm::mat4 matrix = glm::mat4(1.f);
};

class Doodads {
	slk::SLK doodads_slk;
	slk::SLK doodads_meta_slk;
	slk::SLK destructibles_slk;
	slk::SLK destructibles_meta_slk;

	std::vector<Doodad> doodads;

	std::unordered_map<std::string, std::shared_ptr<StaticMesh>> id_to_mesh;

	std::shared_ptr<Shader> shader;
public:
	bool load(BinaryReader& reader, Terrain& terrain);
	void load_destructible_modifications(BinaryReader& reader);
	void load_doodad_modifications(BinaryReader& reader);
	void create();
	void render();
};