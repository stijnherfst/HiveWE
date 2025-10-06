export module RenderNode;

import MDX;
import MathOperations;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;

export struct RenderNode {
	const mdx::Node* node;
	glm::vec3 pivot;

	bool dont_inherit_translation;
	bool dont_inherit_scaling;
	bool dont_inherit_rotation;

	bool billboarded;
	bool billboardedX;
	bool billboardedY;
	bool billboardedZ;

	RenderNode() = default;
	RenderNode(const mdx::Node& node, const glm::vec3 pivot) {
		this->node = &node;
		this->pivot = pivot;

		dont_inherit_translation = node.flags & mdx::Node::Flags::dont_inherit_translation;
		dont_inherit_rotation = node.flags & mdx::Node::Flags::dont_inherit_rotation;
		dont_inherit_scaling = node.flags & mdx::Node::Flags::dont_inherit_scaling;
		billboarded = node.flags & mdx::Node::Flags::billboarded;
		billboardedX = node.flags & mdx::Node::Flags::billboarded_lock_x;
		billboardedY = node.flags & mdx::Node::Flags::billboarded_lock_y;
		billboardedZ = node.flags & mdx::Node::Flags::billboarded_lock_z;
	}
};