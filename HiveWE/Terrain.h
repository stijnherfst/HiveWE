#pragma once

// These classes will be rewritten to use SOA instead of AOS

struct Corner {
	float ground_height;
	float water_height;
	bool map_edge;

	int ground_texture;

	bool ramp;
	bool blight;
	bool water;
	bool boundary;
	bool cliff = false;

	int ground_variation;
	int cliff_variation;

	int cliff_texture;
	int layer_height;
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

class Terrain {
public:
	char tileset;
	std::vector<std::string> tileset_ids;
	std::vector<std::string> cliffset_ids;

	int width;
	int height;

	//int tiles_width;
	//int tiles_height;

	glm::vec2 offset;

	std::vector<std::vector<Corner>> corners;

	// Ground
	std::shared_ptr<Shader> ground_shader;
	std::map<std::string, size_t> ground_texture_to_id;
	std::vector<std::shared_ptr<GroundTexture>> ground_textures;
	std::unordered_map<std::string, TilePathingg> pathing_options;

	GLuint ground_height;
	GLuint ground_corner_height;
	GLuint ground_texture_data;
	GLuint pathing_map_texture;

	std::vector<float> ground_heights;
	std::vector<float> ground_corner_heights;
	std::vector<glm::u16vec4> ground_texture_list;

	int variation_size = 64;
	int blight_texture;

	slk::SLK terrain_slk;
	slk::SLK cliff_slk;

	// Cliffs
	std::vector <glm::ivec3> cliffs;
	std::map<std::string, int> path_to_cliff;
	std::map<std::string, int> cliff_variations;
	std::vector<int> cliff_to_ground_texture;
	
	std::shared_ptr<Shader> cliff_shader;
	std::vector<std::shared_ptr<CliffMesh>> cliff_meshes;
	std::vector<std::shared_ptr<Texture>> cliff_textures;

	GLuint cliff_texture_array;
	
	int cliff_texture_size = 256;

	// Water
	float min_depth = 10.f / 128;
	float deeplevel = 64.f / 128;
	float maxdepth = 72.f / 128;

	glm::vec4 shallow_color_min;
	glm::vec4 shallow_color_max;
	glm::vec4 deep_color_min;
	glm::vec4 deep_color_max;

	float height_offset;
	int water_textures_nr;
	int animation_rate;
	
	std::vector<float> water_heights;
	std::vector<uint8_t> water_exists_data;
	GLuint water_height;
	GLuint water_exists;


	std::vector<std::shared_ptr<Texture>> water_textures;
	std::shared_ptr<Shader> water_shader;

	float current_texture = 1.f;
	GLuint water_texture_array;

	~Terrain();

	void create();
	bool load(BinaryReader& reader);
	void save();
	void render();

	void change_tileset(const std::vector<std::string>& new_tileset_ids, const std::vector<int>& new_to_old);

	static float corner_height(Corner corner);
	float corner_height(int x, int y) const;
	float corner_water_height(Corner corner) const;

	int real_tile_texture(int x, int y);
	int get_tile_variation(int ground_texture, int variation);
	glm::u16vec4 get_texture_variations(int x, int y);
};