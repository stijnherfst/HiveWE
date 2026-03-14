#pragma once

#include <QRect>
#include <string>
#include <vector>
#include <algorithm>

// Forward declaration to avoid circular dependency
class TerrainBrush;

/// Base class for all terrain operators, such as texture painter, cliff tools etc...
class TerrainOperator {
	friend class TerrainBrush;

  public:
	enum class brush_type {
		corner,
		cell
	};

	TerrainOperator(TerrainBrush* brush, brush_type brush_type) : brush(brush) {
		set_brush_type(brush_type);
	}

	virtual void apply_begin(const QRect& area, int center_x, int center_y) = 0;
	virtual void apply(const QRect& area, double frame_delta, QRect& updated_area) = 0;
	virtual void apply_end() = 0;

	/// This function determines whether two operators can be used simultaneously.
	/// For example, texture and cliff operators
	virtual bool can_combine_with(TerrainOperator* other) = 0;

	/// Checks whether the operator is active or not
	bool is_enabled() const {
		return is_active;
	}

	/// Changes brush type
	void set_brush_type(brush_type brush_type);

  protected:
	TerrainBrush* brush;

  private:
	bool is_active = false;
	bool center_on_tile_corner;
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

	HeightOperator(TerrainBrush* brush) : TerrainOperator(brush, brush_type::corner) {}

	void apply_begin(const QRect& area, int center_x, int center_y) override;
	void apply(const QRect& area, double frame_delta, QRect& updated_area) override;
	void apply_end() override;
	bool can_combine_with(TerrainOperator* other) override;

  private:
	float deformation_height_ground;
	float deformation_height_water;
};

class TextureOperator: public TerrainOperator {
  public:
	std::string tile_id;

	TextureOperator(TerrainBrush* brush) : TerrainOperator(brush, brush_type::corner) {}

	void apply_begin(const QRect& area, int center_x, int center_y) override;
	void apply(const QRect& area, double frame_delta, QRect& updated_area) override;
	void apply_end() override;
	bool can_combine_with(TerrainOperator* other) override;
};

class CliffOperator: public TerrainOperator {
  public:
	CliffOperator(TerrainBrush* brush) : TerrainOperator(brush, brush_type::corner) {}

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
	void apply(const QRect& area, double frame_delta, QRect& updated_area) override;
	void apply_end() override;
	bool can_combine_with(TerrainOperator* other) override;

  private:
	int layer_height = 0;
};

class CellOperator: public TerrainOperator {
  public:
	CellOperator(TerrainBrush* brush) : TerrainOperator(brush, brush_type::cell) {}

	enum class cell_operation {
		add_water,
		remove_water,
		add_boundary,
		remove_boundary,
		add_hole,
		remove_hole
	};
	cell_operation cell_operation_type = cell_operation::add_boundary;

	void apply_begin(const QRect& area, int center_x, int center_y) override;
	void apply(const QRect& area, double frame_delta, QRect& updated_area) override;
	void apply_end() override;
	bool can_combine_with(TerrainOperator* other) override;

  private:
	/// Water "ground zero" level in wc3
	static constexpr float WATER_GROUND_ZERO = 0.7f;

	/// Starting water level when using Add Water tool
	static constexpr float WATER_HEIGHT = 0.25f;

	float water_height;
};
