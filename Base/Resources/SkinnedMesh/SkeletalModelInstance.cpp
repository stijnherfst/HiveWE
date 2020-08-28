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

	// Advance current frame
	const mdx::Sequence& sequence = model->sequences[sequence_index];
	if (sequence.flags & mdx::Sequence::non_looping) {
		current_frame = std::min<int>(current_frame + delta * 1000.0, sequence.end_frame);
	} else {
		current_frame += delta * 1000.0;
		if (current_frame > sequence.end_frame) {
			current_frame = sequence.start_frame;
		}
	}

	// Advance keyframe index for all applicable nodes
	//for (auto& node : renderNodes) {
	//	const auto& header = node.node.animated_data.track<glm::vec3>(mdx::TrackTag::KGTR);
	//	
	//	if (header.tracks[node.current.right].frame <= current_frame) {
	//		// If we are at the end we stop since we rely on calling code to loop
	//		if (node.current.right == node.current.sequence_end) {
	//			continue;
	//		}

	//		node.current.left++;
	//		node.current.right++;
	//	}		
	//}

	updateNodes();
	recompute_sequence = false;
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
		//bool objectVisible = node.node.getVisibility(*this) >= MAGIC_RENDER_SHOW_CONSTANT;
		bool objectVisible = true;
		node.visible = (!node.parent || node.parent->visible) && objectVisible;

		if (node.visible) {
			node.localLocation = getValue<glm::vec3>(node.node.animated_data, mdx::TrackTag::KGTR, TRANSLATION_IDENTITY);
			node.localRotation = getValue<glm::quat>(node.node.animated_data, mdx::TrackTag::KGRT, ROTATION_IDENTITY);
			node.localScale = getValue<glm::vec3>(node.node.animated_data, mdx::TrackTag::KGSC, SCALE_IDENTITY);

			//node.localLocation = matrixEaterInterpolate(node.node.KGTR, current_frame, TRANSLATION_IDENTITY);
			//node.localRotation = matrixEaterInterpolate(node.node.KGRT, current_frame, ROTATION_IDENTITY);
			//node.localScale = matrixEaterInterpolate(node.node.KGSC, current_frame, SCALE_IDENTITY);

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

void SkeletalModelInstance::set_sequence(int sequence_index) {
	
}

glm::vec3 SkeletalModelInstance::get_geoset_animation_color(mdx::GeosetAnimation& animation) {
		return animation.color;
	if (animation.animated_data.has_track(mdx::TrackTag::KGAC)) {
		auto& track = animation.animated_data.track<glm::vec3>(mdx::TrackTag::KGAC);
		return matrixEaterInterpolate<glm::vec3>(track, current_frame, animation.color);
	} else {
	}
}

float SkeletalModelInstance::get_geoset_animation_visiblity(mdx::GeosetAnimation& animation) {
		return animation.alpha;
	if (animation.animated_data.has_track(mdx::TrackTag::KGAO)) {
		auto& track = animation.animated_data.track<float>(mdx::TrackTag::KGAO);
		return matrixEaterInterpolate<float>(track, current_frame, animation.alpha);
	} else {
	}
}

float SkeletalModelInstance::get_layer_visiblity(mdx::Layer& layer) {
		return layer.alpha;
	if (layer.animated_data.has_track(mdx::TrackTag::KMTA)) {
		auto& track = layer.animated_data.track<float>(mdx::TrackTag::KMTA);
		return matrixEaterInterpolate<float>(track, current_frame, layer.alpha);
	} else {
	}
}

template <typename T>
T SkeletalModelInstance::getValue(mdx::AnimatedData& animated_data, mdx::TrackTag tag, const T& defaultValue) {
	if (animated_data.has_track(tag)) {
		auto& track = animated_data.track<T>(tag);
		return matrixEaterInterpolate(track, current_frame, defaultValue);
	} else {
		return defaultValue;
	}
}

template glm::vec3 SkeletalModelInstance::getValue(mdx::AnimatedData& animated_data, mdx::TrackTag tag, const glm::vec3& defaultValue);
template glm::quat SkeletalModelInstance::getValue(mdx::AnimatedData& animated_data, mdx::TrackTag tag, const glm::quat& defaultValue);

template <typename T>
#pragma optimize("", off)
T SkeletalModelInstance::matrixEaterInterpolate(mdx::TrackHeader<T>& header, int current_frame, const T& defaultValue) {
	if (header.tracks.empty()) {
		return defaultValue;
	}

	if (header.global_sequence_ID >= 0) {
		// ToDo handle global sequences
		return defaultValue;
	}

	if (sequence_index == -1) {
		// ToDo what??
		return defaultValue;
	}

	mdx::Sequence& sequence = model->sequences[sequence_index];

	if (recompute_sequence) {
		header.current.start = -1;
		header.current.right = -1;
		for (int i = 0; i < header.tracks.size(); i++) {
			const mdx::Track<T>& track = header.tracks[i];
			if (track.frame >= sequence.start_frame && header.current.start == -1) {
				header.current.start = i;
			}

			if (track.frame <= current_frame) {
				header.current.left = i;
			}

			if (track.frame >= current_frame && header.current.right == -1) {
				header.current.right = i;
			}

			if (track.frame <= sequence.end_frame) {
				header.current.end = i;
			} else {
				break;
			}
		}
	}

	// Detect if we looped
	if (header.tracks[header.current.left].frame > current_frame) {
		header.current.left = header.current.start;
		header.current.right = header.current.start + 1;
	}

	// Scan till we find two tracks
	while (header.tracks[header.current.right].frame < current_frame) {
		header.current.left = header.current.right;
		header.current.right++;

		// Reached last keyframe, loop around with start frame
		if (header.current.right > header.current.end) {
			break;
		}
	}

	const bool past_end = header.tracks[header.current.end].frame < current_frame;
	const bool before_start = header.tracks[header.current.start].frame > current_frame;
	if (past_end || before_start) {
		header.current.left = header.current.end;
		header.current.right = header.current.start;
	}

	const T ceilInTan = header.tracks[header.current.right].inTan;
	const T floorOutTan = header.tracks[header.current.left].outTan;

	int floorTime = header.tracks[header.current.left].frame;
	int ceilTime = header.tracks[header.current.right].frame;
	T floorValue = header.tracks[header.current.left].value;
	T ceilValue = header.tracks[header.current.right].value;

	float t = std::clamp((current_frame - floorTime) / static_cast<float>(ceilTime - floorTime), 0.f, 1.f);

	return interpolate(floorValue, floorOutTan, ceilInTan, ceilValue, t, header.interpolation_type);


	//int sequenceStart;
	//int sequenceEnd;
	//if (header.global_sequence_ID >= 0 && model->global_sequences.size()) {
	//	sequenceStart = 0;
	//	sequenceEnd = model->global_sequences[header.global_sequence_ID];
	//	if (sequenceEnd == 0) {
	//		current_frame = 0;
	//	} else {
	//		current_frame = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % sequenceEnd;
	//	}
	//} else if (model->sequences.size() && sequence_index != -1) {
	//	mdx::Sequence& sequence = model->sequences[sequence_index];
	//	sequenceStart = sequence.start_frame;
	//	sequenceEnd = sequence.end_frame;
	//} else {
	//	return defaultValue;
	//}

	//int ceilIndex = -1;
	//int floorIndex = 0;
	//// ToDo "if global seq" check is here in MXE java

	//int floorAnimStartIndex = 0;
	//int floorAnimEndIndex = 0;

	//// get floor:
	//for (int i = 0; i < header.tracks.size(); i++) {
	//	const mdx::Track<T>& track = header.tracks[i];
	//	if (track.frame <= sequenceStart) {
	//		floorAnimStartIndex = i;
	//	}
	//	if (track.frame <= current_frame) {
	//		floorIndex = i;
	//	}
	//	if (track.frame >= current_frame && ceilIndex == -1) {
	//		ceilIndex = i;
	//	}
	//	if (track.frame <= sequenceEnd) {
	//		floorAnimEndIndex = i;
	//	} else {
	//		// end of our sequence
	//		break;
	//	}
	//}
	//if (ceilIndex == -1) {
	//	ceilIndex = header.tracks.size() - 1;
	//}

	//// end get floor
	//if (ceilIndex < floorIndex) {
	//	ceilIndex = floorIndex;
	//	// was a problem in matrix eater, different impl, not problem here?
	//}

	//T floorInTan{};
	//T floorOutTan{};
	//T floorValue{};
	//T ceilValue{};
	//int floorIndexTime;
	//int ceilIndexTime;

	//floorValue = header.tracks[floorIndex].value;
	//if (header.interpolation_type > 1) {
	//	floorInTan = header.tracks[floorIndex].inTan;
	//	floorOutTan = header.tracks[floorIndex].outTan;
	//}
	//ceilValue = header.tracks[ceilIndex].value;
	//floorIndexTime = header.tracks[floorIndex].frame;
	//ceilIndexTime = header.tracks[ceilIndex].frame;
	//if (ceilIndexTime < sequenceStart) {
	//	return defaultValue;
	//}
	//if (floorIndexTime > sequenceEnd) {
	//	return defaultValue;
	//}

	//if (floorIndexTime < sequenceStart && ceilIndexTime > sequenceEnd) {
	//	return defaultValue;
	//} else if (floorIndexTime < sequenceStart) {
	//	if (header.tracks[floorAnimEndIndex].frame == sequenceEnd) {
	//		// no "floor" frame found, but we have a ceil frame,
	//		// so the prev frame is a repeat of animation's end
	//		// placed at the beginning
	//		floorIndex = floorAnimEndIndex;
	//		floorValue = header.tracks[floorAnimEndIndex].value;
	//		floorIndexTime = sequenceStart;
	//		if (header.interpolation_type > 1) {
	//			floorInTan = header.tracks[floorAnimEndIndex].inTan;
	//			floorOutTan = header.tracks[floorAnimEndIndex].outTan;
	//		}
	//	} else {
	//		floorValue = defaultValue;
	//		floorInTan = floorOutTan = defaultValue;
	//		floorIndexTime = sequenceStart;
	//	}
	//} else if (ceilIndexTime > sequenceEnd || (ceilIndexTime < current_frame && header.tracks[floorAnimEndIndex].frame < current_frame)) {
	//	// if we have a floor frame but the "ceil" frame is after end of sequence,
	//	// or our ceil frame is before our time, meaning that we're at the end of the
	//	// entire timeline, then we need to inject a "ceil" frame at end of sequence
	//	if (header.tracks[floorAnimStartIndex].frame == sequenceStart) {
	//		ceilValue = header.tracks[floorAnimStartIndex].value;
	//		ceilIndex = floorAnimStartIndex;
	//		ceilIndexTime = sequenceStart;
	//	}
	//	// for the else case here, Matrix Eater code says to leave it blank,
	//	// example model is Water Elemental's birth animation, to verify behavior
	//}
	//if (floorIndex == ceilIndex) {
	//	return floorValue;
	//}
	//const T ceilInTan = header.tracks[ceilIndex].inTan;
	//float t = std::clamp((current_frame - floorIndexTime) / (float)(ceilIndexTime - floorIndexTime), 0.f, 1.f);

	//return interpolate(floorValue, floorOutTan, ceilInTan, ceilValue, t, header.interpolation_type);
}
#pragma optimize("", on) 

template glm::vec3 SkeletalModelInstance::matrixEaterInterpolate(mdx::TrackHeader<glm::vec3>& header, int current_frame, const glm::vec3& defaultValue);
template glm::quat SkeletalModelInstance::matrixEaterInterpolate(mdx::TrackHeader<glm::quat>& header, int current_frame, const glm::quat& defaultValue);