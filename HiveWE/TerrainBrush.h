#pragma once

class TerrainBrush : public Brush {
public:
	bool apply_texture = false;
	bool apply_height = false;
	bool apply_cliff = false;
	bool apply_tile_pathing = true;
	bool apply_cliff_pathing = true;

	bool enforce_water_height_limits = true;

	std::string tile_id;

	enum class deformation {
		raise,
		lower,
		plateau,
		ripple,
		smooth
	};
	deformation deformation_type = deformation::plateau;

	int cliff_id = 0;

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
	cliff_operation cliff_operation_type = cliff_operation::level;

	TerrainBrush();
	void check_nearby(int begx, int begy, int i, int j, QRect& area) const;
	void apply() override;
	void apply_end() override;

	int get_random_variation() const;
private:
	// Total sum 570
	const std::tuple<int, int> variation_chances[18] = {
		{ 0, 85 },
		{ 16, 85 },
		{ 0, 85 },
		{ 1, 10 },
		{ 2, 4 },
		{ 3, 1 },
		{ 4, 85 },
		{ 5, 10 },
		{ 6, 4 },
		{ 7, 1 },
		{ 8, 85 },
		{ 9, 10 },
		{ 10, 4 },
		{ 11, 1 },
		{ 12, 85 },
		{ 13, 10 },
		{ 14, 4 },
		{ 15, 1 }
	};

	bool brush_hold = false;
	int layer_height = 0;
	int deformation_height = 0;
};