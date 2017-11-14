#pragma once

struct Corner {
	int ground_height;
	int water_height;
	int map_edge;

	int ground_texture;
	
	bool ramp;
	bool blight;
	bool water;
	bool boundary;
	bool cliff;

	int ground_variation;
	int cliff_variation;

	int cliff_texture;
	int layer_height;

	float height() {
		return (ground_height - 0x2000 + (layer_height - 2) * 0x0200) / 512.0;
	}

	float height_water() {
		return (water_height - 0x2000 + (layer_height - 2) * 0x0200) / 512.0;
	}
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
	GLuint vertexBuffer;
	GLuint uvBuffer;
	GLuint indexBuffer;

	std::vector <glm::vec3> vertices;
	std::vector <glm::vec3> uvs;
	std::vector <glm::ivec3> indices;

	std::vector<std::shared_ptr<Texture>> ground_textures;
	std::shared_ptr<Shader> ground_shader;

	GLuint ground_texture_array;

	int variation_width = 64;
	int variation_height = 64;
	int blight_texture;

	// Cliffs
	std::vector <glm::ivec3> cliffs;
	std::map<std::string, int> path_to_cliff;
	
	std::vector<std::shared_ptr<Texture>> cliff_textures;
	std::vector<std::shared_ptr<StaticMesh>> cliff_meshes;
	std::shared_ptr<Shader> cliff_shader;

	std::vector<GLuint> cliff_texture_list;
	
	int cliff_texture_size = 256;

	// Water
	GLuint water_vertexBuffer;
	GLuint water_uvBuffer;
	GLuint water_indexBuffer;

	std::vector <glm::vec3> water_vertices;
	std::vector <glm::vec2> water_uvs;
	std::vector <glm::ivec3> water_indices;

	const int water_textures_nr = 45;
	std::vector<std::shared_ptr<Texture>> water_textures;
	std::shared_ptr<Shader> water_shader;

	int current_texture = 0;
	GLuint water_texture_array;

	void create();
	bool load(std::vector<uint8_t> data);
	void render();

	int get_tile_variation(Corner& tile_corner);
	std::vector<std::tuple<int, int>> get_texture_variations(Corner& topLeft, Corner& topRight, Corner& bottomLeft, Corner& bottomRight);
};