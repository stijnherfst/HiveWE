#pragma once

class TerrainBrush : public Brush {
public:
	std::string tile_id;
	void apply() override;
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

};