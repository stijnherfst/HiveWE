module;

#include <QRectF>

export module Doodad;

import std;
import SkinnedMesh;
import PathingTexture;
import SkeletalModelInstance;
import Terrain;
import SLK;
import Globals;
import ResourceManager;
import Utilities;
import <glm/glm.hpp>;
import <glm/gtc/quaternion.hpp>;

export struct Doodad {
	static inline int auto_increment;

	std::string id;
	std::string skin_id;
	int variation = 0;
	glm::vec3 position = glm::vec3(0.f);
	glm::vec3 scale = glm::vec3(1.f);
	float angle = 0.f;

	enum class State {
		invisible_non_solid,
		visible_non_solid,
		visible_solid
	};
	State state = State::visible_solid;
	int life = 100;

	int item_table_pointer = -1;
	std::vector<ItemSet> item_sets;

	int creation_number;

	// Auxiliary data
	SkeletalModelInstance skeleton;
	std::shared_ptr<SkinnedMesh> mesh;
	std::shared_ptr<PathingTexture> pathing;
	glm::vec3 color = glm::vec3(1.f);

	void init(const std::string_view id, const std::shared_ptr<SkinnedMesh> mesh, const Terrain& terrain) {
		this->id = id;
		this->skin_id = id;
		this->mesh = mesh;

		skeleton = SkeletalModelInstance(mesh->mdx);
		// Get pathing map
		const bool is_doodad = doodads_slk.row_headers.contains(id);
		const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

		pathing.reset();
		const auto path = slk.data<std::string_view>("pathtex", id);
		const auto trimmed_path = trimmed(path);
		if (!trimmed_path.empty() && trimmed_path != "none" && trimmed_path != "_") {
			try {
				pathing = resource_manager.load<PathingTexture>(trimmed_path);
			} catch (const std::exception& e) {
				std::println("Error load pathing texture for doodad with ID: {} and error {}", id, e.what());
			}
		}

		update(terrain);
	}

	void update(const Terrain& terrain) {
		float base_scale = 1.f;
		float max_roll;
		float max_pitch;
		if (doodads_slk.row_headers.contains(id)) {
			color.r = doodads_slk.data<float>("vertr" + std::to_string(variation + 1), id) / 255.f;
			color.g = doodads_slk.data<float>("vertg" + std::to_string(variation + 1), id) / 255.f;
			color.b = doodads_slk.data<float>("vertb" + std::to_string(variation + 1), id) / 255.f;
			max_roll = doodads_slk.data<float>("maxroll", id);
			max_pitch = doodads_slk.data<float>("maxpitch", id);
			base_scale = doodads_slk.data<float>("defscale", id);
		} else {
			color.r = destructibles_slk.data<float>("colorr", id) / 255.f;
			color.g = destructibles_slk.data<float>("colorg", id) / 255.f;
			color.b = destructibles_slk.data<float>("colorb", id) / 255.f;
			max_roll = destructibles_slk.data<float>("maxroll", id);
			max_pitch = destructibles_slk.data<float>("maxpitch", id);
		}

		glm::quat rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));

		constexpr float SAMPLE_RADIUS = 32.f / 128.f;

		// A negative value is a way for the user to set a specific (positive) value
		// A positive value indicates to follow the terrain
		float pitch = 0.f;
		if (max_pitch < 0.f) {
			pitch = max_pitch;
		} else if (max_pitch > 0.f) {
			const float forward_x = position.x + (SAMPLE_RADIUS * std::cos(angle));
			const float forward_y = position.y + (SAMPLE_RADIUS * std::sin(angle));
			const float backward_x = position.x - (SAMPLE_RADIUS * std::cos(angle));
			const float backward_y = position.y - (SAMPLE_RADIUS * std::sin(angle));

			const float height1 = terrain.interpolated_height(backward_x, backward_y, false);
			const float height2 = terrain.interpolated_height(forward_x, forward_y, false);

			pitch = std::clamp(std::atan2(height2 - height1, SAMPLE_RADIUS * 2.f), -pitch, pitch);
		}
		rotation *= glm::angleAxis(-pitch, glm::vec3(0, 1, 0));

		// A negative value is a way for the user to set a specific (positive) value
		// A positive value indicates to follow the terrain
		float roll = 0.f;
		if (max_roll < 0.f) {
			roll = -max_roll;
		} else if (max_roll > 0.f) {
			const float left_of_angle = angle + (3.1415926535 / 2.0);
			const float forward_x = position.x + (SAMPLE_RADIUS * std::cos(left_of_angle));
			const float forward_y = position.y + (SAMPLE_RADIUS * std::sin(left_of_angle));
			const float backward_x = position.x - (SAMPLE_RADIUS * std::cos(left_of_angle));
			const float backward_y = position.y - (SAMPLE_RADIUS * std::sin(left_of_angle));

			const float height1 = terrain.interpolated_height(backward_x, backward_y, false);
			const float height2 = terrain.interpolated_height(forward_x, forward_y, false);

			roll = std::clamp(atan2(height2 - height1, SAMPLE_RADIUS * 2.f), -roll, roll);
		}
		rotation *= glm::angleAxis(roll, glm::vec3(1, 0, 0));

		skeleton.update_location(position, rotation, (base_scale * scale) / 128.f);
	}

	[[nodiscard]] QRect get_pathing_bounding_box() const {
		if (!pathing) {
			return {};
		}

		const int rotation = glm::degrees(angle);
		const int rotated_width = rotation % 180 ? pathing->width : pathing->height;
		const int rotated_height = rotation % 180 ? pathing->height : pathing->width;
		const int x = position.x * 4 - rotated_width / 2;
		const int y = position.y * 4 - rotated_height / 2;
		return QRect{x, y, rotated_width, rotated_height};
	}

	static glm::vec2 acceptable_position(
		const glm::vec2 position,
		const std::shared_ptr<PathingTexture>& pathing,
		const float angle,
		const bool force_grid_aligned = false
	) {
		if (!pathing) {
			if (force_grid_aligned) {
				return glm::round(position * 2.f) * 0.5f;
			} else {
				return position;
			}
		}

		const int rotation = glm::degrees(angle);
		const int rotated_width = rotation % 180 ? pathing->width : pathing->height;
		const int rotated_height = rotation % 180 ? pathing->height : pathing->width;

		glm::vec2 extra_offset(0.0f);
		if (rotated_width % 4 != 0) {
			extra_offset.x = 0.25f;
		}

		if (rotated_height % 4 != 0) {
			extra_offset.y = 0.25f;
		}

		return glm::round((position + extra_offset) * 2.f) * 0.5f - extra_offset;
	}

	static float acceptable_angle(
		const std::string_view id,
		const std::shared_ptr<PathingTexture>& pathing,
		const float current_angle,
		const float target_angle
	) {
		float fixed_rotation = 0.0;
		if (doodads_slk.row_headers.contains(id)) {
			fixed_rotation = doodads_slk.data<float>("fixedrot", id);
		} else {
			fixed_rotation = destructibles_slk.data<float>("fixedrot", id);
		}

		// Negative values indicate free rotation, positive is a fixed angle
		if (fixed_rotation >= 0.0) {
			return glm::radians(fixed_rotation);
		}

		if (pathing) {
			if (pathing->width == pathing->height && pathing->homogeneous) {
				return target_angle;
			} else {
				return (static_cast<int>((target_angle + glm::pi<float>() * 0.25f) / (glm::pi<float>() * 0.5f)) % 4) * glm::pi<float>()
					* 0.5f;
			}
		} else {
			return target_angle;
		}
	}
};

export struct SpecialDoodad {
	std::string id;
	int variation;
	glm::vec3 position;
	glm::vec3 old_position;

	// Auxiliary data
	glm::mat4 matrix = glm::mat4(1.f);
	SkeletalModelInstance skeleton;
	std::shared_ptr<SkinnedMesh> mesh;
	std::shared_ptr<PathingTexture> pathing;
};