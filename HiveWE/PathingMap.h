#pragma once

class PathingMap {
	struct PathCell {
		bool walkable;
		bool flyable;
		bool buildable;
		bool blight;
		bool water;

		glm::vec3 color() {
			return { walkable ? 0 : 255, flyable ? 0 : 255,  buildable ? 0 : 255 };
		}
	};

	int width;
	int height;

	std::vector<PathCell> pathing_map;

	GLuint vertex_buffer;
	GLuint color_buffer;
	GLuint index_buffer;

	std::vector <glm::vec3> vertices;
	std::vector <glm::vec3> colors;
	std::vector <glm::ivec3> indices;
	std::shared_ptr<Shader> shader;


	public:
	bool load(BinaryReader& reader, Terrain& terrain);
	void create(Terrain& terrain);
	void render();
};