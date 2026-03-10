export module GameCameras;

import std;
import types;
import BinaryReader;
import Hierarchy;

export struct GameCamera {
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

export class GameCameras {
  public:
	std::vector<GameCamera> cameras;

	void load(int game_version_major, int game_version_minor, float terrain_offset_x, float terrain_offset_y) {
		BinaryReader reader = hierarchy.map_file_read("war3map.w3c").value();

		int version = reader.read<u32>();
		if (version != 0) {
			std::cout << "Unknown war3map.w3c version: " << version << " Attempting to load but may crash\n";
		}

		cameras.resize(reader.read<u32>());
		for (auto& i : cameras) {
			// change coordinate system to [0, terrain_width] x [0, terrain_height]
			// as used by other objects in HiveWE
			i.target_x = (reader.read<float>() - terrain_offset_x) / 128.f;
			i.target_y = (reader.read<float>() - terrain_offset_y) / 128.f;
			i.z_offset = reader.read<float>() / 128.f;

			i.rotation = reader.read<float>();
			i.angle_of_attack = reader.read<float>();
			i.distance = reader.read<float>();
			i.roll = reader.read<float>();
			i.fov = reader.read<float>();
			i.far_z = reader.read<float>();
			i.near_z = reader.read<float>();

			if (game_version_major * 100 + game_version_minor >= 131) {
				i.local_pitch = reader.read<float>();
				i.local_yaw = reader.read<float>();
				i.local_roll = reader.read<float>();
			}
			i.name = reader.read_c_string();
		}
	}

	void save() {}

	void remove_camera(GameCamera* camera) {
		const auto iterator = cameras.begin() + std::distance(cameras.data(), camera);
		cameras.erase(iterator);
	}

	void remove_cameras(const std::unordered_set<GameCamera*>& list) {
		std::erase_if(cameras, [&](GameCamera& camera) {
			return list.contains(&camera);
		});
	}
};
