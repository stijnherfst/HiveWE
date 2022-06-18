#pragma once

#include <vector>
#include <memory>

#include "SkinnedMesh.h"
#include "Utilities.h"
import PathingTexture;

#include "Terrain.h"
#include "TerrainUndo.h"

#include <SkeletalModelInstance.h>

struct Doodad {
	static inline int auto_increment;

	std::string id;
	std::string skin_id;
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

	// Auxiliary data
	glm::mat4 matrix = glm::mat4(1.f);
	SkeletalModelInstance skeleton;
	std::shared_ptr<SkinnedMesh> mesh;
	std::shared_ptr<PathingTexture> pathing;
	glm::vec3 color;

	void update();
	static float acceptable_angle(std::string_view id, std::shared_ptr<PathingTexture> pathing, float current_angle, float target_angle);
};

struct SpecialDoodad {
	std::string id;
	int variation;
	glm::vec3 position;
	glm::vec3 old_position;

	// Auxiliary data
	glm::mat4 matrix = glm::mat4(1.f);
	SkeletalModelInstance skeleton;
	std::shared_ptr<SkinnedMesh> mesh;
	std::shared_ptr<PathingTexture> pathing;
};

class Doodads {
	std::unordered_map<std::string, std::shared_ptr<SkinnedMesh>> id_to_mesh;

	static constexpr int write_version = 8;
	static constexpr int write_subversion = 11;
	static constexpr int write_special_version = 0;

public:
	std::vector<SpecialDoodad> special_doodads;
	std::vector<Doodad> doodads;

	bool load();
	void save() const;
	void create();
	void render();

	Doodad& add_doodad(std::string id, int variation, glm::vec3 position);
	Doodad& add_doodad(Doodad doodad);

	void remove_doodad(Doodad* doodad);

	std::vector<Doodad*> query_area(const QRectF& area);
	void remove_doodads(const std::vector<Doodad*>& list);

	void update_doodad_pathing(const std::vector<Doodad*>& target_doodads);
	void update_doodad_pathing(const std::vector<Doodad>& target_doodads);
	void update_doodad_pathing(const QRectF& area);

	void update_special_doodad_pathing(const QRectF& area);

	void process_doodad_field_change(const std::string& id, const std::string& field);
	void process_destructible_field_change(const std::string& id, const std::string& field);

	std::shared_ptr<SkinnedMesh> get_mesh(std::string id, int variation);
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