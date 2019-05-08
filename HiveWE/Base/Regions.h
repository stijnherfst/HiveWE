#pragma once

struct Region {
	float left;
	float right;
	float top;
	float bottom;
	std::string name;
	int creation_number;
	std::string weather_id;
	std::string ambient_id;
	glm::vec3 color;
};

class Regions {
	static constexpr int write_version = 5;


public:
	std::vector<Region> regions;

	bool load(BinaryReader& reader);
	void save() const;
};