export module RenderNode;

import MDX;
import MathOperations;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;

export struct RenderNode {
	mdx::Node* node;
	RenderNode* parent;
	glm::vec3 pivot;

	bool dontInheritTranslation;
	bool dontInheritScaling;
	bool dontInheritRotation;

	bool billboarded;
	bool billboardedX;
	bool billboardedY;
	bool billboardedZ;

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