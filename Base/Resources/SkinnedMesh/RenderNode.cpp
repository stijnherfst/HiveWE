#include "RenderNode.h"

#include "Utilities.h"

RenderNode::RenderNode(mdx::Node node, glm::vec3 pivot) {
	this->node = node;
	this->pivot = pivot;

	dontInheritTranslation = node.flags & mdx::Node::Flags::dont_inherit_translation;
	dontInheritRotation = node.flags & mdx::Node::Flags::dont_inherit_rotation;
	dontInheritScaling = node.flags & mdx::Node::Flags::dont_inherit_scaling;
	billboarded = node.flags & mdx::Node::Flags::billboarded;
	billboardedX = node.flags & mdx::Node::Flags::billboarded_lock_x;
	billboardedY = node.flags & mdx::Node::Flags::billboarded_lock_y;
	billboardedZ = node.flags & mdx::Node::Flags::billboarded_lock_z;
}

void RenderNode::recalculateTransformation() {
	fromRotationTranslationScaleOrigin(rotation, position, scale, worldMatrix, pivot);

	if (parent) {
		worldMatrix = parent->worldMatrix * worldMatrix;
	}

	// Pretty sure that dontInheritTranslation/Rotation are really broken since they don't take the pivot into account
	/*if (parent) {
		glm::quat computedRotation;
		glm::vec3 computedScaling;
		glm::vec3 computedLocation;

		if (dontInheritTranslation) {
			computedLocation = position * parent->inverseWorldLocation;
		} else {
			computedLocation = position;
		}

		if (dontInheritRotation) {
			computedRotation = rotation * parent->inverseWorldRotation;
		} else {
			computedRotation = rotation;
		}

		if (dontInheritScaling) {
			computedScaling = parent->inverseWorldScale * scale;
			worldScale = scale;
		} else {
			computedScaling = scale;
			worldScale = parent->worldScale * scale;
		}
		fromRotationTranslationScaleOrigin(computedRotation, computedLocation, computedScaling, worldMatrix, pivot);
		
		worldMatrix = parent->worldMatrix * worldMatrix;
		worldRotation = parent->worldRotation * rotation;
	} else {
		fromRotationTranslationScaleOrigin(rotation, position, scale, worldMatrix, pivot);
		worldRotation = rotation;
		worldScale = scale;
	}
	
	inverseWorldRotation = -worldRotation;
	inverseWorldRotation.w = worldRotation.w;

	inverseWorldScale = 1.f / worldScale;
	worldLocation = worldMatrix[3];
	inverseWorldLocation = -worldLocation;*/
}