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

	TerrainOperator(brush_type brush_type) {
		if (brush_type == brush_type::cell) {
			center_on_tile_corner = false;
		} else if (brush_type == brush_type::corner) {
			center_on_tile_corner = true;
		}
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
		smooth,
		none
	};
	deformation deformation_type = deformation::none;

	HeightOperator() : TerrainOperator(brush_type::corner) {}

	void apply_begin(const QRect& area, int center_x, int center_y) override;
	void apply(const QRect& area, double frame_delta, QRect& updated_area) override;
	void apply_end() override;
	bool can_combine_with(TerrainOperator* other) override;

  private:
	float deformation_height;
};

class TextureOperator: public TerrainOperator {
  public:
	std::string tile_id;

	TextureOperator() : TerrainOperator(brush_type::cell) {}

	void apply_begin(const QRect& area, int center_x, int center_y) override;
	void apply(const QRect& area, double frame_delta, QRect& updated_area) override;
	void apply_end() override;
	bool can_combine_with(TerrainOperator* other) override;
};

class CliffOperator: public TerrainOperator {
  public:
	CliffOperator() : TerrainOperator(brush_type::corner) {}

	enum class cliff_operation {
		lower2,
		lower1,
		level,
		raise1,
		raise2,
		deep_water,
		shallow_water,
		ramp,
		none
	};
	cliff_operation cliff_operation_type = cliff_operation::none;

	int cliff_id = 0;

	void apply_begin(const QRect& area, int center_x, int center_y) override;
	void apply(const QRect& area, double frame_delta, QRect& updated_area) override;
	void apply_end() override;
	bool can_combine_with(TerrainOperator* other) override;

  private:
	int layer_height = 0;
};
