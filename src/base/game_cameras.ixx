export module GameCameras;

import std;
import types;
import BinaryReader;
import Hierarchy;

export struct GameCamera {
	f32 target_x;
	f32 target_y;
	f32 z_offset;
	f32 rotation;
	f32 angle_of_attack;
	f32 distance;
	f32 roll;
	f32 fov;
	f32 far_z;
	f32 near_z;

	f32 local_pitch;
	f32 local_yaw;
	f32 local_roll;

	f32 dof_distance;
	f32 dof_scale;
	f32 pos_absolute_z;
	std::string name;
	u32 free_camera;
};

export class GameCameras {
  public:
	std::vector<GameCamera> cameras;

	void load(const i32 game_version_major, const i32 game_version_minor, const f32 terrain_offset_x, const f32 terrain_offset_y) {
		BinaryReader reader = hierarchy.map_file_read("war3map.w3c").value();

		const int version = reader.read<u32>();
		if (version != 0 && version != 3) {
			std::cout << "Unknown war3map.w3c version: " << version << " Attempting to load but may crash\n";
		}

		cameras.resize(reader.read<u32>());
		for (auto& i : cameras) {
			// change coordinate system to [0, terrain_width] x [0, terrain_height]
			// as used by other objects in HiveWE
			i.target_x = (reader.read<f32>() - terrain_offset_x) / 128.f;
			i.target_y = (reader.read<f32>() - terrain_offset_y) / 128.f;
			i.z_offset = reader.read<f32>() / 128.f;

			i.rotation = reader.read<f32>();
			i.angle_of_attack = reader.read<f32>();
			i.distance = reader.read<f32>();
			i.roll = reader.read<f32>();
			i.fov = reader.read<f32>();
			i.far_z = reader.read<f32>();
			i.near_z = reader.read<f32>();

			if (game_version_major * 100 + game_version_minor >= 131) {
				i.local_pitch = reader.read<f32>();
				i.local_yaw = reader.read<f32>();
				i.local_roll = reader.read<f32>();
			}
			if (version >= 3) {
				i.dof_distance = reader.read<f32>();
				i.dof_scale = reader.read<f32>();
				i.pos_absolute_z = reader.read<f32>();
			}

			i.name = reader.read_c_string();

			if (version >= 3) {
				i.free_camera = reader.read<u32>();
			}
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
