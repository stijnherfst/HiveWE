#pragma once

class PathingMap {
	int width;
	int height;

	std::vector<uint8_t> pathing_map;

	GLuint pathing_vertex_buffer;
	GLuint pathing_color_buffer;
	GLuint pathing_index_buffer;

	std::vector <glm::vec3> pathing_vertices;
	std::vector <glm::vec3> pathing_colors;
	std::vector <glm::ivec3> pathing_indices;
	std::shared_ptr<Shader> pathing_shader;

	public:
	bool load(BinaryReader& reader);
	void create();
};