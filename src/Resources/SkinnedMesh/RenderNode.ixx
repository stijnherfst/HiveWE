module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

export module RenderNode;

import MDX;
import MathOperations;

export struct RenderNode {
	mdx::Node* node;
	RenderNode* parent;
	glm::vec3 pivot;
	// local space
	//glm::vec3 position;
	//glm::quat rotation = glm::quat(1.f, 0.f, 0.f, 0.f);
	//glm::vec3 scale = glm::vec3(1, 1, 1);

	// world space (not including game unit X/Y, mdx-m3-viewer's name)
	//glm::vec3 worldLocation;
	//glm::quat worldRotation = glm::quat(1.f, 0.f, 0.f, 0.f);
	//glm::vec3 worldScale = glm::vec3(1, 1, 1);
	//glm::mat4 worldMatrix = glm::mat4(1.f);
	// inverse world space
	//glm::vec3 inverseWorldLocation;
	//glm::quat inverseWorldRotation;
	//glm::vec3 inverseWorldScale;
	// cached flags from node, could be removed for RAM
	bool dontInheritTranslation;
	bool dontInheritScaling;
	bool dontInheritRotation;

	bool billboarded;
	bool billboardedX;
	bool billboardedY;
	bool billboardedZ;
	// state flags
	bool visible;

	RenderNode() = default;
	RenderNode(mdx::Node& node, glm::vec3 pivot) {
		this->node = &node;
		this->pivot = pivot;

		dontInheritTranslation = node.flags & mdx::Node::Flags::dont_inherit_translation;
		dontInheritRotation = node.flags & mdx::Node::Flags::dont_inherit_rotation;
		dontInheritScaling = node.flags & mdx::Node::Flags::dont_inherit_scaling;
		billboarded = node.flags & mdx::Node::Flags::billboarded;
		billboardedX = node.flags & mdx::Node::Flags::billboarded_lock_x;
		billboardedY = node.flags & mdx::Node::Flags::billboarded_lock_y;
		billboardedZ = node.flags & mdx::Node::Flags::billboarded_lock_z;
	}
};