#pragma once

#include <memory>

#include <QObject>
#include <QOpenGLFunctions_4_5_Core>
#include <QRect>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>

#include <bullet/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include "btBulletDynamicsCommon.h"

#include "GroundTexture.h"
#include "CliffMesh.h"
#include "Texture.h"
#include "BinaryReader.h"
#include "Shader.h"
#include "SLK.h"

#include "TerrainUndo.h"

struct Corner {
	bool map_edge;

	int ground_texture;

	float height;
	float water_height;
	bool ramp;
	bool blight;
	bool water;
	bool boundary;
	bool cliff = false;
	bool romp = false;

	int ground_variation;
	int cliff_variation;

	int cliff_texture;
	int layer_height;

	float final_ground_height() const;
	float final_water_height() const;
};

struct TilePathingg {
	bool unwalkable = false;
	bool unflyable = false;
	bool unbuildable = false;

	uint8_t mask() const {
		uint8_t mask = 0;
		mask |= unwalkable ? 0b00000010 : 0;
		mask |= unflyable ? 0b00000100 : 0;
		mask |= unbuildable ? 0b00001000 : 0;
		return mask;
	}
};

class Terrain : public QObject {
	Q_OBJECT

	static constexpr int write_version = 11;

	// Sequential versions for GPU uploading
	std::vector<float> ground_heights;
	std::vector<float> ground_corner_heights;
	std::vector<glm::u16vec4> ground_texture_list;
	std::vector<float> water_heights;
	std::vector<unsigned char> water_exists_data;

	
	btHeightfieldTerrainShape* collision_shape;
	btRigidBody* collision_body;
public:
	char tileset;
	std::vector<std::string> tileset_ids;
	std::vector<std::string> cliffset_ids;

	int width;
	int height;
	glm::vec2 offset;

	// Ground
	std::shared_ptr<Shader> ground_shader;
	std::map<std::string, int> ground_texture_to_id;
	std::vector<std::shared_ptr<GroundTexture>> ground_textures;
	std::unordered_map<std::string, TilePathingg> pathing_options;

	// GPU textures
	GLuint ground_height;
	GLuint ground_corner_height;
	GLuint ground_texture_data;
	GLuint water_height;
	GLuint water_exists;

	std::vector<std::vector<Corner>> corners;
	// For undo/redo operations
	std::vector<std::vector<Corner>> old_corners;

	int variation_size = 64;
	int blight_texture;

	slk::SLK terrain_slk;
	slk::SLK cliff_slk;

	// Cliffs
	std::vector<glm::ivec3> cliffs;
	std::map<std::string, int> path_to_cliff;
	std::map<std::string, int> cliff_variations;
	std::vector<int> cliff_to_ground_texture;
	
	std::shared_ptr<Shader> cliff_shader;
	std::vector<std::shared_ptr<CliffMesh>> cliff_meshes;
	std::vector<std::shared_ptr<Texture>> cliff_textures;

	GLuint cliff_texture_array;
	
	int cliff_texture_size = 256;

	// Water
	float min_depth = 10.f / 128.f;
	float deeplevel = 64.f / 128.f;
	float maxdepth = 72.f / 128.f;

	glm::vec4 shallow_color_min;
	glm::vec4 shallow_color_max;
	glm::vec4 deep_color_min;
	glm::vec4 deep_color_max;

	float water_offset;
	int water_textures_nr;
	int animation_rate;

	std::shared_ptr<Shader> water_shader;

	float current_texture = 1.f;
	GLuint water_texture_array;

	~Terrain();

	bool load(BinaryReader& reader);
	void create();
	void save() const;
	void render() const;

	void change_tileset(const std::vector<std::string>& new_tileset_ids, std::vector<int> new_to_old);

	int real_tile_texture(int x, int y) const;
	int get_tile_variation(int ground_texture, int variation) const;
	glm::u16vec4 get_texture_variations(int x, int y) const;

	float interpolated_height(float x, float y) const;

	//bool is_corner_ramp_mesh(int x, int y);
	bool is_corner_ramp_entrance(int x, int y);
	//bool is_corner_cliff(int x, int y);

	Texture minimap_image();

	enum class undo_type {
		texture,
		height,
		cliff,
		water
	};

	void new_undo_group();
	void add_undo(const QRect& area, undo_type type);

	void upload_ground_heights() const;
	void upload_corner_heights() const;
	void upload_ground_texture() const;
	void upload_water_exists() const;
	void upload_water_heights() const;

	void update_ground_heights(const QRect& area);
	void update_ground_textures(const QRect& area);
	void update_water(const QRect& area);
	void update_cliff_meshes(const QRect& area);

	void update_minimap();
signals:
	void minimap_changed(Texture minimap);
};

// Undo/redo structures
class TerrainGenericAction : public TerrainUndoAction {
public:
	QRect area;
	std::vector<Corner> old_corners;
	std::vector<Corner> new_corners;
	Terrain::undo_type undo_type;

	void undo() override;
	void redo() override;
};