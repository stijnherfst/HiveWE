#pragma once

struct GameCamera {
	float target_x;
	float target_y;
	float z_offset;
	float rotation;
	float angle_of_attack;
	float distance;
	float roll;
	float fov;
	float far_z;
	float near_z; // ?
	std::string name;
};

class GameCameras {
public:
	std::vector<GameCamera> cameras;

	void load(BinaryReader& reader);
	void save();
};