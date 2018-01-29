#pragma once

class Brush {
public:
	enum class Type {
		pathing,
		terrain
	};
	
	glm::vec2 position;
	glm::vec2 uv_offset;
	int size = 0;
	int granularity = 4;
	Type type = Type::terrain;

	std::vector<glm::u8vec4> brush;
	GLuint brush_texture;

	std::shared_ptr<Shader> shader;

	void create();
	void set_position(const glm::vec2 position);
	void set_size(const int size);

	void render(Terrain& terrain);

	virtual void apply(Terrain& terrain) = 0;
};

class PathingBrush : public Brush {
	enum class Operation {
		replace,
		add,
		remove
	};

	Operation operation = Operation::replace;

	void apply(Terrain& terrain) override;
};