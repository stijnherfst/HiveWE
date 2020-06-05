#pragma once

#include <vector>
#include <string>

#include "BinaryReader.h"


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
	float near_z;
	float local_roll;
	float local_pitch;
	float local_yaw;
	std::string name;
};

class GameCameras {
public:
	std::vector<GameCamera> cameras;

	void load();
	void save();
};