#pragma once

struct Index {
	unsigned int first, second, third;
};

struct Corner {
	int ground_height;
	int water_height;
	int map_edge;

	int ground_texture;
	
	bool ramp;
	bool blight;
	bool water;
	bool boundary;

	int ground_variation;
	int cliff_variation;

	int cliff_texture;
	int layer_height;
};

struct TerrainTexture {
	bool extended;
	
	// icon?
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
	

	std::vector<std::shared_ptr<Texture>> textures;
	int variation_width = 64;
	int variation_height = 64;
	int blight_texture;

	GLuint vertexBuffer;
	GLuint uvBuffer;
	GLuint indexBuffer;

	std::vector <glm::vec3> vertices;
	std::vector <glm::vec3> uvs;
	std::vector <Index> indices;

	GLuint textureArray;


	void create();
	bool load(std::vector<uint8_t> data);
	void render();

	int get_tile_variation(Corner& tile_corner);
	std::vector<std::tuple<int, int>> get_texture_variations(Corner& topLeft, Corner& topRight, Corner& bottomLeft, Corner& bottomRight);
};