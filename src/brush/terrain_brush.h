#pragma once

#include <string>

#include "brush.h"
#include "doodads.h"

class TerrainBrush : public Brush {
public:
	bool apply_texture = false;
	bool apply_height = false;
	bool apply_cliff = false;
	bool apply_tile_pathing = true;
	bool apply_cliff_pathing = true;
	bool apply_water_pathing = true;

	bool enforce_water_height_limits = true;
	bool change_doodad_heights = true;
	bool relative_cliff_heights = false;

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

	bool dragging = false;
	bool dragged = false;

	TerrainBrush();

	void mouse_release_event(QMouseEvent* event) override;
	void mouse_press_event(QMouseEvent* event, double frame_delta) override;
	void mouse_move_event(QMouseEvent* event, double frame_delta) override;

	void check_nearby(int begx, int begy, int i, int j, QRect& area) const;

	void apply_begin() override;
	void apply(double frame_delta) override;
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

	int layer_height = 0;
	float deformation_height = 0.f;

	QRect texture_height_area;
	QRect cliff_area;
	std::vector<Doodad> pre_change_doodads;
	std::map<int, Doodad> post_change_doodads;

};