#pragma once

#include <QMouseEvent>
#include <QKeyEvent>

#include <glm/glm.hpp>

#include <memory>
#include <string_view>
#include <vector>

struct WorldEditContext;

/// The 2D overlays brushes draw on the map, as model matrices mapping the unit square [0, 1]^2 into the world.
/// The map renders them with the camera; brushes stay independent of the graphics API.
struct BrushDrawList {
	/// Green rectangle outlines, e.g. the drag selection box
	std::vector<glm::mat4> selection_rectangles;
	/// Green rings inscribed in the square, marking selected objects
	std::vector<glm::mat4> selection_circles;
};

class Brush: public QObject {
	Q_OBJECT

  public:
	enum class Shape {
		square,
		circle,
		diamond
	};

	enum class Mode {
		placement,
		selection,
		pasting
	};

	enum class Type {
		corner,
		cell
	};

	/// Bindless slot of the brush shape texture the terrain shaders overlay at the brush position
	uint32_t brush_texture = 0;
	/// Owns the brush shape texture. Type-erased because this header can't import the Vulkan modules
	std::shared_ptr<void> brush_texture_owner;

	Brush();

	virtual glm::vec2 get_position() const;
	virtual void set_size(glm::ivec2 size);
	virtual void set_shape(Shape shape);
	virtual void increase_size(int size);
	virtual void decrease_size(int size);
	virtual bool contains(glm::ivec2 pos) const;

	virtual void switch_mode();

	Mode get_mode() const {
		return mode;
	}

	virtual void key_press_event(WorldEditContext& ctx, const QKeyEvent* event);
	virtual void key_release_event(WorldEditContext& ctx, const QKeyEvent* event) {}
	virtual void mouse_move_event(WorldEditContext& ctx, const QMouseEvent* event, double frame_delta);
	virtual void mouse_press_event(WorldEditContext& ctx, const QMouseEvent* event, double frame_delta);
	virtual void mouse_release_event(WorldEditContext& ctx, const QMouseEvent* event);

	virtual void delete_selection() {}

	virtual void copy_selection() {}

	virtual void cut_selection() {}

	virtual void clear_selection() {}

	virtual void place_clipboard(WorldEditContext& ctx) {}

	virtual void clear_clipboard() {}

	/// Queues brush previews with the render manager and adds the brush's overlays to `draw_list`
	void render(BrushDrawList& draw_list);
	virtual void render_selector(BrushDrawList& draw_list) const;

	virtual void render_selection(BrushDrawList& draw_list) const {}

	virtual void render_clipboard() {}

	virtual void render_brush();

	virtual bool can_place() {
		return true;
	}

	virtual void apply_begin(WorldEditContext& ctx) {}

	virtual void apply(WorldEditContext& ctx, double frame_delta) = 0;

	virtual void apply_end(WorldEditContext& ctx) {}

  protected:
	Shape shape = Shape::circle;
	Mode mode = Mode::placement;
	Mode return_mode = Mode::placement;
	Type brush_type = Type::corner;

	/// The color used to render the brush which currently mimics the WC3 default
	glm::u8vec4 brush_color = {0, 255, 0, 128};

	/// How many quarter tiles fit into one size unit for this brush. Terrain brush is 4, pathing brush is 1.
	int size_granularity = 1;
	/// The granularity of the grid that the position will be snapped to. There will be 1 / position_granularity cells in a tile
	float position_granularity = 1.f;
	/// Size in 1/4ths of a tile (corresponding to the pathing map tile size which is the smallest)
	glm::ivec2 size = glm::ivec2(1);

	bool selection_started = false;
	glm::vec3 selection_start;

  public slots:

	virtual void unselect_id(std::string_view id) {}

  signals:
	void size_changed(glm::ivec2 size);
	void selection_changed();
};
