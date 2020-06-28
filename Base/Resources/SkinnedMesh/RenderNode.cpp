#include "RenderNode.h"

#include "Utilities.h"

RenderNode::RenderNode(mdx::Node node, glm::vec3 pivot) {
	this->node = node;
	this->pivot = pivot;

	dontInheritScaling = node.flags & 0x1;
	billboarded = node.flags & 0x8;
	billboardedX = node.flags & 0x10;
	billboardedY = node.flags & 0x20;
	billboardedZ = node.flags & 0x40;
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

	// Matrix Eater functionality begins below.
	// Everything above was only in the JS.
	// Apparently I completely removed and omitted all logic
	// relating to dirty nodes, perhaps because I wanted
	// correct behavior and did not want to implement the
	// performance optimization that Ghostwolf and I discussed
	// about dirty nodes. Shouldn't that mean the ME impl is buggy?
	// Surely I didn't invent some way to omit the above?

	// ToDo remove above comment for being too verbose.

	if (dirty) {
		dirty = false;

		if (parent) {
			glm::vec3 computedLocation;
			glm::vec3 computedScaling;
			computedLocation = localLocation;

			// ToDo Ghostwolf has some commented-out code
			// here to process "dontInheritRotation"
			// and "dontInheritTranslation" flags.
			// Matrix Eater completely skipped it so it's probably
			// bugged, too??? Needs a test.

			if (dontInheritScaling) {
				computedScaling = parent->inverseWorldScale * localScale;
				worldScale = localScale;
			} else {
				computedScaling = localScale;
				worldScale = parent->worldScale * localScale;
			}
			fromRotationTranslationScaleOrigin(localRotation, computedLocation, computedScaling, localMatrix, pivot);

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