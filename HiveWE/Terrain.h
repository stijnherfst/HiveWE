#pragma once

struct Corner {
	float ground_height;
	float water_height;
	int map_edge;

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

class Terrain {

public:
	char tileset;
	std::vector<std::string> tileset_ids;
	std::vector<std::string> cliffset_ids;

	int width;
	int height;

	float offset_x;
	float offset_y;

	std::vector <Corner> corners;

	// Ground
	GLuint vertex_buffer;
	GLuint uv_buffer;
	GLuint index_buffer;

	std::vector <glm::vec3> vertices;
	std::vector <glm::vec3> uvs;
	std::vector <glm::ivec3> indices;

	std::map<std::string, size_t> ground_texture_to_id;
	std::vector<std::shared_ptr<Texture>> ground_textures;
	std::shared_ptr<Shader> ground_shader;

	GLuint ground_texture_array;

	int variation_width = 64;
	int variation_height = 64;
	int blight_texture;

	// Cliffs
	std::vector <glm::ivec3> cliffs;
	std::map<std::string, int> path_to_cliff;
	std::vector<int> cliff_to_ground_texture;
	std::map<std::string, int> cliff_variations;
	
	std::vector<std::shared_ptr<Texture>> cliff_textures;
	std::vector<std::shared_ptr<StaticMesh>> cliff_meshes;
	std::shared_ptr<Shader> cliff_shader;

	std::vector<GLuint> cliff_texture_list;
	
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
	
	GLuint water_vertex_buffer;
	GLuint water_uv_buffer;
	GLuint water_index_buffer;
	GLuint water_color_buffer;

	std::vector <glm::vec3> water_vertices;
	std::vector <glm::vec2> water_uvs;
	std::vector <glm::ivec3> water_indices;
	std::vector <glm::vec4> water_colors;

	std::vector<std::shared_ptr<Texture>> water_textures;
	std::shared_ptr<Shader> water_shader;

	float current_texture = 1.f;
	GLuint water_texture_array;

	void create();
	bool load(std::vector<uint8_t> data);
	void render();

	int real_tile_texture(int x, int y);
	float corner_height(Corner corner) const;
	float corner_water_height(Corner corner) const;

	int get_tile_variation(const Corner& tile_corner);
	std::vector<std::tuple<int, int>> get_texture_variations(int x, int y);
};