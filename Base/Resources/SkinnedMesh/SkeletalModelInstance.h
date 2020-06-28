#pragma once

#include <chrono>
#include <vector>

#include "MDX.h"

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "RenderNode.h"

// Ghostwolf mentioned this to me once, so I used it,
// as 0.75, experimentally determined as a guess at
// whatever WC3 is doing. Do more reserach if necessary?
#define MAGIC_RENDER_SHOW_CONSTANT 0.75

class SkeletalModelInstance {
  private:
	void setupHierarchy();

  public:
	std::shared_ptr<mdx::MDX> model;
	int sequence_index; // can be -1 if not animating
	int current_frame;
	glm::quat inverseCameraRotation = glm::quat(0.f, 0.f, 0.f, 1.0f);
	glm::quat inverseCameraRotationXSpin;
	glm::quat inverseCameraRotationYSpin;
	glm::quat inverseCameraRotationZSpin;

	glm::mat4 matrix = glm::mat4(1.f);
	glm::vec3 position;
	float facingAngle;
	glm::quat inverseInstanceRotation;

	std::vector<RenderNode> renderNodes;

	SkeletalModelInstance() = default;
	explicit SkeletalModelInstance(std::shared_ptr<mdx::MDX> model);

	void updateLocation(glm::vec3& position, float angle, glm::vec3& scale);

	void update(double delta);

	void updateNodes(bool forced);
};