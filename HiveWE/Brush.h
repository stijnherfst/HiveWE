#pragma once

class Brush {
public:
	enum class Shape {
		square,
		circle,
		diamond
	};

	int granularity = 1;
	bool uv_offset_locked = false;
	glm::ivec2 uv_offset;
	glm::vec2 brush_offset = { 0, 0 };

	void create();
	virtual void set_position(const glm::vec2& position);
	glm::vec2 get_position() const;
	void set_size(int size);
	void set_shape(Shape shape);
	void increase_size(int size);
	void decrease_size(int size);
	bool contains(int x, int y) const;

	void render(Terrain& terrain) const;

	virtual void apply() = 0;
	virtual void apply_end() {};
protected:
	Shape shape = Shape::circle;

	int size = 0;
	glm::ivec2 position;

	std::vector<glm::u8vec4> brush;
	GLuint brush_texture;

	std::shared_ptr<Shader> shader;
};