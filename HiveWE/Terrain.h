#pragma once

struct Corner {
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

class Terrain : public QObject {
	Q_OBJECT

	static const int write_version = 11;

public:
	char tileset;
	std::vector<std::string> tileset_ids;
	std::vector<std::string> cliffset_ids;

	int width;
	int height;
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

	float water_offset;
	int water_textures_nr;
	int animation_rate;
	
	std::vector<float> water_heights;
	std::vector<unsigned char> water_exists_data;
	GLuint water_height;
	GLuint water_exists;


	std::vector<std::shared_ptr<Texture>> water_textures;
	std::shared_ptr<Shader> water_shader;

	float current_texture = 1.f;
	GLuint water_texture_array;

	~Terrain();

	void create();
	bool load(BinaryReader& reader);
	void save() const;
	void render() const;

	void change_tileset(const std::vector<std::string>& new_tileset_ids, std::vector<int> new_to_old);

	float corner_height(int x, int y) const;
	float corner_water_height(const int x, const int y) const;

	int real_tile_texture(int x, int y) const;
	int get_tile_variation(int ground_texture, int variation) const;
	glm::u16vec4 get_texture_variations(int x, int y) const;

	Texture minimap_image();

signals:
	void minimap_changed(Texture minimap);
};