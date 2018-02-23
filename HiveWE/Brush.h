#pragma once

class Brush {
public:
	enum class Type {
		pathing,
		terrain
	};
	
	glm::ivec2 position;
	glm::ivec2 uv_offset;
	int size = 0;
	int granularity = 4;
	Type type = Type::terrain;

	std::vector<glm::u8vec4> brush;
	GLuint brush_texture;
	uint8_t brush_mask;

	std::shared_ptr<Shader> shader;

	void create();
	void set_position(const glm::vec2& position);
	void set_size(int size);

	void render(Terrain& terrain) const;

	virtual void apply(PathingMap& pathing) = 0;
};

class PathingBrush : public Brush {
public:
	enum class Operation {
		replace,
		add,
		remove
	};

	Operation operation = Operation::replace;

	void apply(PathingMap& pathing) override;
};