#pragma once

#include <QRect>

#include <cstdint>
#include <vector>

#include "brush.h"

class PathingBrush : public Brush {
public:
	enum class Operation {
		replace,
		add,
		remove
	};

	uint8_t brush_mask = 0b00000000;

	Operation operation = Operation::replace;

	QRect applied_area;

	PathingBrush();

	void apply_begin() override;
	void apply(double frame_delta) override;
	void apply_end() override;

	void add_pathing_undo(const QRect& area);

private:
	std::vector<uint8_t> old_pathing_cells_static;
};