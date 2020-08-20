#include "SkeletalModelInstance.h"

#include "Camera.h"
#include "Utilities.h"

SkeletalModelInstance::SkeletalModelInstance(std::shared_ptr<mdx::MDX> model) {
	this->model = model;

	size_t node_count = model->bones.size() +
		model->lights.size() +
		model->help_bones.size() +
		model->attachments.size() +
		model->emitters1.size() +
		model->emitters2.size() +
		model->ribbons.size() +
		model->eventObjects.size() +
		model->collisionShapes.size() +
		model->corn_emitters.size();
	
	// ToDo: for each camera: add camera source node to renderNodes
	renderNodes.resize(node_count);
	setupHierarchy();
	// ToDo: for each camera: add camera target node to renderNodes
}

void SkeletalModelInstance::setupHierarchy() {
	// Leaking is bad, if you leak, you are serving the Demon Lord.

	model->forEachNode([&](mdx::Node& object) {
		// Seen it happen with Emmitter1, is this an error in the model?
		// ToDo purge (when adding a validation layer or just crashing)
		if (object.id == -1) {
			return;
		}

		glm::vec3 pivot;
		if (object.id < model->pivots.size()) {
			pivot = model->pivots[object.id];
		} else {
			pivot = glm::vec3(0, 0, 0);
		}

		RenderNode renderNode = RenderNode(object, pivot);
		if (object.parent_id != -1) {
			renderNode.parent = &renderNodes[object.parent_id];
		} else {
			renderNode.parent = nullptr;
		}

		renderNodes[object.id] = renderNode;
	});
}

void SkeletalModelInstance::updateLocation(glm::vec3 position, float angle, const glm::vec3& scale) {
	this->position = position;
	this->facingAngle = angle;
	glm::vec3 origin = glm::vec3(0, 0, 0);
	glm::vec3 stackScale = scale;
	glm::vec3 axis = glm::vec3(0, 0, 1);
	glm::quat rotation = glm::angleAxis(angle, axis);
	inverseInstanceRotation.x = -rotation.x;
	inverseInstanceRotation.y = -rotation.y;
	inverseInstanceRotation.z = -rotation.z;
	inverseInstanceRotation.w = rotation.w;
	fromRotationTranslationScaleOrigin(rotation, position, stackScale, matrix, origin);
}

void SkeletalModelInstance::update(double delta) {
	if (model->sequences.empty() || sequence_index == -1) {
		return;
	}

	const mdx::Sequence& sequence = model->sequences[sequence_index];
	if (sequence.flags & mdx::SequenceFlags::non_looping) {
		current_frame = std::min<int>(current_frame + delta * 1000.0, sequence.interval_end);
	} else {
		current_frame += delta * 1000.0;
		if (current_frame > sequence.interval_end) {
			current_frame = sequence.interval_start;
		}
	}
	updateNodes();
}

void SkeletalModelInstance::updateNodes() {
	//if (sequence_index < 0) {
	//	// no animation loaded, render in base position
	//	for (auto& node : renderNodes) {
	//		node.resetTransformation();
	//		node.worldMatrix = glm::mat4(1.0f);
	//	}
	//} else {


	assert(sequence_index >= 0 && sequence_index < model->sequences.size());

	// update skeleton to position based on animation @ time
	for (auto& node : renderNodes) {
		bool objectVisible = node.node.getVisibility(*this) >= MAGIC_RENDER_SHOW_CONSTANT;
		node.visible = (!node.parent || node.parent->visible) && objectVisible;

		if (node.visible) {
			node.localLocation = node.node.getValue<glm::vec3>(mdx::TrackTag::KGTR, *this, TRANSLATION_IDENTITY);
			node.localRotation = node.node.getValue<glm::quat>(mdx::TrackTag::KGRT, *this, ROTATION_IDENTITY);
			node.localScale = node.node.getValue<glm::vec3>(mdx::TrackTag::KGSC, *this, SCALE_IDENTITY);

			if (node.billboarded || node.billboardedX) {
				// Cancel the parent's rotation
				if (node.parent) {
					node.localRotation = node.parent->inverseWorldRotation * inverseInstanceRotation;
				} else {
					node.localRotation = inverseInstanceRotation;
				}

				node.localRotation *= camera->decomposed_rotation;
			}

			node.recalculateTransformation();
		}
	}
}
