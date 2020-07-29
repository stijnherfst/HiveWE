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

//void RenderNode::setTransformation(glm::vec3 location, glm::quat rotation, glm::vec3 scale) {
//	localLocation = location;
//	localRotation = rotation;
//	localScale = scale;
//
//	dirty = true;
//}

void RenderNode::resetTransformation() {
	localLocation = glm::vec3(0.f);
	localRotation = glm::quat(1.f, 0.f, 0.f, 0.f);
	localScale = glm::vec3(1.f, 1.f, 1.f);
	dirty = true;
}

void RenderNode::recalculateTransformation() {
	// Need to update if this node is dirty, or if its parent was dirty.
	wasDirty = dirty;

	if (parent) {
		dirty = dirty || parent->wasDirty;
	}

	wasDirty = dirty;

	if (dirty) {
		dirty = false;

		if (parent) {
			glm::quat computedRotation;
			glm::vec3 computedScaling;
			glm::vec3 computedLocation;

			if (dontInheritTranslation) {
				computedLocation = localLocation * parent->inverseWorldLocation;
			} else {
				computedLocation = localLocation;
			}

			if (dontInheritRotation) {
				computedRotation = localRotation * parent->inverseWorldRotation;
			} else {
				computedRotation = localRotation;
			}

			if (dontInheritScaling) {
				computedScaling = parent->inverseWorldScale * localScale;
				worldScale = localScale;
			} else {
				computedScaling = localScale;
				worldScale = parent->worldScale * localScale;
			}
			fromRotationTranslationScaleOrigin(computedRotation, computedLocation, computedScaling, localMatrix, pivot);

			worldMatrix = parent->worldMatrix * localMatrix;
			worldRotation = parent->worldRotation * localRotation;
		} else {
			fromRotationTranslationScaleOrigin(localRotation, localLocation, localScale, localMatrix, pivot);
			worldMatrix = localMatrix;
			worldRotation = localRotation;
			worldScale = localScale;
		}

		// Inverse world rotation (ToDo use overloaded assignment operator)
		inverseWorldRotation.x = -worldRotation.x;
		inverseWorldRotation.y = -worldRotation.y;
		inverseWorldRotation.z = -worldRotation.z;
		inverseWorldRotation.w = worldRotation.w;

		inverseWorldScale = 1.f / worldScale;
		worldLocation = worldMatrix[3];
		inverseWorldLocation = -worldLocation;
	}
}

void RenderNode::update(void* scene) {
	if (dirty || (parent && parent->wasDirty)) {
		dirty = true; // In this case this node isn't dirty, but the parent was
		wasDirty = true;
		recalculateTransformation();
	} else {
		wasDirty = false;
	}
}