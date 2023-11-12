#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <filesystem>

#include <QObject>

#include <glm/glm.hpp>

import BinaryReader;
import Utilities;
import SkinnedMesh;
import SkeletalModelInstance;
import TerrainUndo;

struct Unit {
	static inline int auto_increment;

	std::string id;
	int variation = 0;
	glm::vec3 position;
	float angle;
	glm::vec3 scale;

	std::string skin_id;

	uint8_t flags = 2;
	int player = 0;

	uint8_t unknown1 = 0;
	uint8_t unknown2 = 0;
	int health = -1;
	int mana = -1;

	int item_table_pointer = 0xFFFF;
	std::vector<ItemSet> item_sets;

	int gold = 12500;
	float target_acquisition = -1;

	int level = 1;
	int strength = 0;
	int agility = 0;
	int intelligence = 0;

	// Slot, ID
	std::vector<std::pair<uint32_t, std::string>> items;

	// ID, autocast, ability level
	std::vector<std::tuple<std::string, uint32_t, uint32_t>> abilities;

	int random_type = 0;
	std::vector<uint8_t> random;

	int custom_color = -1;
	int waygate = -1;
	int creation_number;

	SkeletalModelInstance skeleton;
	std::shared_ptr<SkinnedMesh> mesh;
	glm::vec3 color;

	Unit() {
		creation_number = ++auto_increment;
	}

	void update();
};

class Units : public QObject {
	Q_OBJECT

	std::unordered_map<std::string, std::shared_ptr<SkinnedMesh>> id_to_mesh;

	static constexpr int write_version = 8;
	static constexpr int write_subversion = 11;

	//static constexpr int mod_table_write_version = 2;
public:
	std::vector<Unit> units;
	std::vector<Unit> items;

	void load();
	void save() const;
	void update_area(const QRect& area);
	void create();

	Unit& add_unit(std::string id, glm::vec3 position);
	Unit& add_unit(Unit unit);

	void remove_unit(Unit* unit);

	std::vector<Unit*> query_area(const QRectF& area);
	void remove_units(const std::unordered_set<Unit*>& list);

	void process_unit_field_change(const std::string& id, const std::string& field);
	void process_item_field_change(const std::string& id, const std::string& field);

	std::shared_ptr<SkinnedMesh> get_mesh(const std::string& id);
};

// Undo/redo structures
class UnitAddAction : public TerrainUndoAction {
public:
	std::vector<Unit> units;

	void undo() override;
	void redo() override;
};

class UnitDeleteAction : public TerrainUndoAction {
public:
	std::vector<Unit> units;

	void undo() override;
	void redo() override;
};

class UnitStateAction : public TerrainUndoAction {
public:
	std::vector<Unit> old_units;
	std::vector<Unit> new_units;

	void undo() override;
	void redo() override;
};