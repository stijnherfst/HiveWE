#pragma once

#include <chrono>
#include <vector>
#include <memory>
#include <unordered_map>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

import RenderNode;
import MDX;


// Ghostwolf mentioned this to me once, so I used it,
// as 0.75, experimentally determined as a guess at
// whatever WC3 is doing. Do more reserach if necessary?
#define MAGIC_RENDER_SHOW_CONSTANT 0.75

// To keep track of what
struct CurrentKeyFrame {
	int start = -1;
	int end = 0;
	int left = 0;
	int right = 0;
};

class SkeletalModelInstance {
  public:
	std::shared_ptr<mdx::MDX> model;

	int sequence_index = 0; // can be -1 if not animating
	int current_frame = 0;

	glm::quat inverseCameraRotationXSpin;
	glm::quat inverseCameraRotationYSpin;
	glm::quat inverseCameraRotationZSpin;

	glm::mat4 matrix = glm::mat4(1.f);
	glm::quat inverseInstanceRotation;

	std::vector<CurrentKeyFrame> current_keyframes;
	std::vector<RenderNode> render_nodes;
	std::vector<glm::mat4> world_matrices;

	SkeletalModelInstance() = default;
	explicit SkeletalModelInstance(std::shared_ptr<mdx::MDX> model);

	void updateLocation(glm::vec3 position, float angle, const glm::vec3& scale);

	void update(double delta);

	void updateNodes();

	void set_sequence(int sequence_index);

	template <typename T>
	void calculate_sequence_extents(const mdx::TrackHeader<T>& header);

	template <typename T>
	void advance_keyframes(const mdx::TrackHeader<T>& header);

	glm::vec3 get_geoset_animation_color(const mdx::GeosetAnimation& animation) const;
	float get_geoset_animation_visiblity(const mdx::GeosetAnimation& animation) const;
	float get_layer_visiblity(const mdx::Layer& layer) const;

	template <typename T>
	T interpolate_keyframes(const mdx::TrackHeader<T>& header, const T& defaultValue) const;
};