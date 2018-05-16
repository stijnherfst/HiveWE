#pragma once

class PathingBrush : public Brush {
public:
	enum class Operation {
		replace,
		add,
		remove
	};

	uint8_t brush_mask = 0b00000000;

	Operation operation = Operation::replace;

	void apply() override;
};