#pragma once

#include <QRect>
#include <string>
#include <vector>
#include <algorithm>

class TerrainBrush;
class Terrain;
struct WorldEditContext;

/// Base class for all terrain operators, such as texture painter, cliff tools etc...
class TerrainOperator {
	friend class TerrainBrush;

  public:
	TerrainOperator(TerrainBrush& brush, Brush::Type type) : brush(&brush) {
		set_brush_type(type);
	}

	virtual void apply_begin(const QRect& area, int center_x, int center_y) = 0;
	virtual QRect apply(const QRect& area, double frame_delta) = 0;
	virtual void apply_end(WorldEditContext& ctx, const QRect& area) = 0;

	/// Checks whether the operator is active or not
	bool is_enabled() const {
		return is_active;
	}

	/// Changes brush type (can be centered on corners or cells)
	void set_brush_type(Brush::Type type);

  protected:
	TerrainBrush* brush;

  private:
	bool is_active = false;
	Brush::Type brush_type;
};

class HeightOperator: public TerrainOperator {
  public:
	enum class deformation {
		raise,
		lower,
		plateau,
		ripple,
		smooth
	};
	deformation deformation_type = deformation::raise;

	HeightOperator(TerrainBrush& brush) : TerrainOperator(brush, Brush::Type::corner) {}

	void apply_begin(const QRect& area, int center_x, int center_y) override;
	QRect apply(const QRect& area, double frame_delta) override;
	void apply_end(WorldEditContext& ctx, const QRect& area) override;

  private:
	float deformation_height_ground;
	float deformation_height_water;
};

class TextureOperator: public TerrainOperator {
  public:
	std::string tile_id;

	TextureOperator(TerrainBrush& brush) : TerrainOperator(brush, Brush::Type::corner) {}

	void apply_begin(const QRect& area, int center_x, int center_y) override;
	QRect apply(const QRect& area, double frame_delta) override;
	void apply_end(WorldEditContext& ctx, const QRect& area) override;
};

class CliffOperator: public TerrainOperator {
  public:
	CliffOperator(TerrainBrush& brush) : TerrainOperator(brush, Brush::Type::corner) {}

	enum class cliff_operation {
		lower2,
		lower1,
		level,
		raise1,
		raise2,
		deep_water,
		shallow_water,
		ramp
	};
	cliff_operation cliff_operation_type = cliff_operation::raise1;

	int cliff_id = 0;

	void apply_begin(const QRect& area, int center_x, int center_y) override;
	QRect apply(const QRect& area, double frame_delta) override;
	void apply_end(WorldEditContext& ctx, const QRect& area) override;

	void check_nearby(const int begx, const int begy, const int i, const int j, QRect& area) const;
	void update_ramp(const int i, const int j, const int horizontal, const int vertical, QRect& area);
	QRect apply_cliffs(const QRect& area, double frame_delta);
	QRect apply_ramps(const QRect& area, double frame_delta);

  private:
	int layer_height = 0;
};

class CellOperator: public TerrainOperator {
  public:
	CellOperator(TerrainBrush& brush) : TerrainOperator(brush, Brush::Type::cell) {}

	enum class cell_operation {
		add_water,
		remove_water,
		add_boundary,
		remove_boundary,

		// future work
		add_hole,
		remove_hole
	};

	void apply_begin(const QRect& area, int center_x, int center_y) override;
	QRect apply(const QRect& area, double frame_delta) override;
	void apply_end(WorldEditContext& ctx, const QRect& area) override;

	void set_operation_type(cell_operation operation);
	cell_operation get_operation_type();

  private:
	/// Water "ground zero" level in wc3
	static constexpr float WATER_GROUND_ZERO = 0.7f;

	/// Starting water level when using Add Water tool
	static constexpr float WATER_HEIGHT = 0.25f;

	/// Returns true if the water is above the ground
	bool water_above_ground(int corner_id) const;

	float water_height;
	cell_operation cell_operation_type = cell_operation::add_boundary;
};
