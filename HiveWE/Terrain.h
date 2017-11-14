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
};

class Terrain {
public:
	char tileset;
	std::vector<std::string> tileset_ids;
	std::vector<std::string> cliffset_ids;

	int width = 3;
	int height = 3;

	float offset_x;
	float offset_y;

	std::vector <Corner> corners;
	
	std::vector<std::shared_ptr<Texture>> ground_textures;
	std::vector<std::shared_ptr<Texture>> cliff_textures;

	int variation_width = 64;
	int variation_height = 64;
	int blight_texture;

	int cliff_texture_size = 256;

	GLuint vertexBuffer;
	GLuint uvBuffer;
	GLuint indexBuffer;

	std::vector <glm::vec3> vertices;
	std::vector <glm::vec3> uvs;
	std::vector <glm::ivec3> indices;

	std::vector<std::shared_ptr<StaticMesh>> cliff_meshes;
	std::vector <glm::ivec3> cliffs;
	std::map<std::string, int> path_to_cliff;

	std::shared_ptr<Shader> ground_shader;
	std::shared_ptr<Shader> cliff_shader;

	GLuint ground_texture_array;
	std::vector<GLuint> cliff_texture_list;


	void create();
	bool load(std::vector<uint8_t> data);
	void render();

	int get_tile_variation(Corner& tile_corner);
	std::vector<std::tuple<int, int>> get_texture_variations(Corner& topLeft, Corner& topRight, Corner& bottomLeft, Corner& bottomRight);
};