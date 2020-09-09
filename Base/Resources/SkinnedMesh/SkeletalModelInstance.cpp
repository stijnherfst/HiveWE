#include "SkeletalModelInstance.h"

#include "Camera.h"
#include "Utilities.h"

SkeletalModelInstance::SkeletalModelInstance(std::shared_ptr<mdx::MDX> model) : model(model) {
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
	render_nodes.resize(node_count);
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
			renderNode.parent = &render_nodes[object.parent_id];
		} else {
			renderNode.parent = nullptr;
		}

		render_nodes[object.id] = renderNode;
	});

	current_keyframes.resize(model->unique_tracks);

	for (auto& i : render_nodes) {
		calculate_sequence_extents(i.node->KGTR);
		calculate_sequence_extents(i.node->KGRT);
		calculate_sequence_extents(i.node->KGSC);
	}

	//for (auto& i : model->materials) {
	//	for (auto& j : i.layers) {
	//		// Add more when used
	//		calculate_sequence_extents(j.KMTA);
	//	}
	//}

	//for (auto& i : model->animations) {
	//	calculate_sequence_extents(i.KGAC);
	//	calculate_sequence_extents(i.KGAO);
	//}
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

	for (auto& i : render_nodes) {
		advance_keyframes(i.node->KGTR);
		advance_keyframes(i.node->KGRT);
		advance_keyframes(i.node->KGSC);
	}

	//for (auto& i : model->materials) {
	//	for (auto& j : i.layers) {
	//		// Add more when used
	//		advance_keyframes(j.KMTA);
	//	}
	//}

	//for (auto& i : model->animations) {
	//	advance_keyframes(i.KGAC);
	//	advance_keyframes(i.KGAO);
	//}

	updateNodes();
}

void SkeletalModelInstance::updateNodes() {
	assert(sequence_index >= 0 && sequence_index < model->sequences.size());

	// update skeleton to position based on animation @ time
	for (auto& node : render_nodes) {
		node.position = interpolate_keyframes(node.node->KGTR, TRANSLATION_IDENTITY);
		node.rotation = interpolate_keyframes(node.node->KGRT, ROTATION_IDENTITY);
		node.scale = interpolate_keyframes(node.node->KGSC, SCALE_IDENTITY);

		//if (node.billboarded || node.billboardedX) {
		//	// Cancel the parent's rotation
		//	if (node.parent) {
		//		node.localRotation = node.parent->inverseWorldRotation * inverseInstanceRotation;
		//	} else {
		//		node.localRotation = inverseInstanceRotation;
		//	}

		//	node.localRotation *= camera->decomposed_rotation;
		//}
 

		node.recalculateTransformation();
	}
}

void SkeletalModelInstance::set_sequence(int sequence_index) {
	
}

template <typename T>
void SkeletalModelInstance::calculate_sequence_extents(mdx::TrackHeader<T>& header) {
	if (header.id == -1) {
		return;
	}
	CurrentKeyFrame& current = current_keyframes[header.id];

	const mdx::Sequence& sequence = model->sequences[sequence_index];

	int local_sequence_start = sequence.start_frame;
	int local_sequence_end = sequence.end_frame;
	int local_current_frame = current_frame;

	if (header.global_sequence_ID >= 0 && model->global_sequences.size()) {
		local_sequence_start = 0;
		local_sequence_end = model->global_sequences[header.global_sequence_ID];

		if (local_sequence_end == 0) {
			local_current_frame = 0;
		} else {
			local_current_frame = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % local_sequence_end;
		}
	}

	current.start = -1;
	current.right = -1;

	for (int i = 0; i < header.tracks.size(); i++) {
		const mdx::Track<T>& track = header.tracks[i];


		if (track.frame > local_sequence_end) {
			break;
		}

		if (track.frame >= local_sequence_start && current.start == -1) {
			current.start = i;
		}

		if (track.frame <= local_current_frame) {
			current.left = i;
		}

		if (track.frame >= local_current_frame && current.right == -1) {
			current.right = i;
		}

		//if (track.frame <= local_sequence_end) {
			current.end = i;
		//}
	}

	//header.current.right = std::min(header.current.left + 1, header.current.end);
}

template void SkeletalModelInstance::calculate_sequence_extents(mdx::TrackHeader<glm::vec3>& header);
template void SkeletalModelInstance::calculate_sequence_extents(mdx::TrackHeader<glm::quat>& header);

template <typename T>
void SkeletalModelInstance::advance_keyframes(mdx::TrackHeader<T>& header) {
	if (header.id == -1) {
		return;
	}
	CurrentKeyFrame& current = current_keyframes[header.id];

	int local_current_frame = current_frame;

	if (header.global_sequence_ID >= 0 && model->global_sequences.size()) {
		int local_sequence_end = model->global_sequences[header.global_sequence_ID];
		if (local_sequence_end == 0) {
			local_current_frame = 0;
		} else {
			local_current_frame = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % local_sequence_end;
		}
	}

	// If there are no tracks in sequence
	if (current.start == -1) {
		return;
	}

	// If there is only 1 track
	if (current.start == current.end) {
		return;
	}

	// Detect if we looped
	if (header.tracks[current.left].frame > local_current_frame) {
		current.left = current.start;
		current.right = current.start + 1;
	}

	// Scan till we find two tracks
	while (header.tracks[current.right].frame < local_current_frame) {
		current.left = current.right;
		current.right++;

		// Reached last keyframe
		if (current.right > current.end) {
			break;
		}
		if (header.tracks[current.right].frame == local_current_frame) {
			current.left = current.right;
		}
	}

	const bool past_end = header.tracks[current.end].frame < local_current_frame;
	const bool before_start = header.tracks[current.start].frame > local_current_frame;
	if (past_end || before_start) {
		current.left = current.end;
		current.right = current.start;
	}
}

template void SkeletalModelInstance::advance_keyframes(mdx::TrackHeader<glm::vec3>& header);
template void SkeletalModelInstance::advance_keyframes(mdx::TrackHeader<glm::quat>& header);

glm::vec3 SkeletalModelInstance::get_geoset_animation_color(mdx::GeosetAnimation& animation) const {
	//return interpolate_keyframes(animation.KGAC, animation.color);
	return animation.color;
}

float SkeletalModelInstance::get_geoset_animation_visiblity(mdx::GeosetAnimation& animation) const {
	//return interpolate_keyframes(animation.KGAO, animation.alpha);
	return animation.alpha;
}

float SkeletalModelInstance::get_layer_visiblity(mdx::Layer& layer) const {
	//return interpolate_keyframes(layer.KMTA, layer.alpha);
	return layer.alpha;
}

template <typename T>
T SkeletalModelInstance::interpolate_keyframes(mdx::TrackHeader<T>& header, const T& defaultValue) const {
	if (header.id == -1) {
		return defaultValue;
	}
	const CurrentKeyFrame& current = current_keyframes[header.id];

#if 1
	int local_current_frame = current_frame;

	if (header.global_sequence_ID >= 0 && model->global_sequences.size()) {
		int local_sequence_end = model->global_sequences[header.global_sequence_ID];
		if (local_sequence_end == 0) {
			local_current_frame = 0;
		} else {
			local_current_frame = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % local_sequence_end;
		}
	}

	// If there are no tracks in sequence
	if (current.start == -1) {
		return defaultValue;
	}

	// If there is only 1 track
	if (current.start == current.end) {
		return header.tracks[current.left].value;
	}

	const T ceilInTan = header.tracks[current.right].inTan;
	const T floorOutTan = header.tracks[current.left].outTan;

	int floorTime = header.tracks[current.left].frame;
	int ceilTime = header.tracks[current.right].frame;
	T floorValue = header.tracks[current.left].value;
	T ceilValue = header.tracks[current.right].value;

	// ToDo Wrapping is wrong around sequence end/start 
	float tt = (local_current_frame - floorTime) / static_cast<float>(ceilTime - floorTime);
	float t = std::clamp(tt, 0.f, 1.f);

	return interpolate(floorValue, floorOutTan, ceilInTan, ceilValue, t, header.interpolation_type);
#else
	if (header.tracks.empty()) {
		return defaultValue;
	}

	int sequenceStart;
	int sequenceEnd;
	if (header.global_sequence_ID >= 0 && model->global_sequences.size()) {
		sequenceStart = 0;
		sequenceEnd = model->global_sequences[header.global_sequence_ID];
		if (sequenceEnd == 0) {
			current_frame = 0;
		} else {
			current_frame = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % sequenceEnd;
		}
	} else if (model->sequences.size() && sequence_index != -1) {
		mdx::Sequence& sequence = model->sequences[sequence_index];
		sequenceStart = sequence.start_frame;
		sequenceEnd = sequence.end_frame;
	} else {
		return defaultValue;
	}

	int ceilIndex = -1;
	int floorIndex = 0;
	// ToDo "if global seq" check is here in MXE java

	int floorAnimStartIndex = 0;
	int floorAnimEndIndex = 0;

	// get floor:
	for (int i = 0; i < header.tracks.size(); i++) {
		const mdx::Track<T>& track = header.tracks[i];
		if (track.frame <= sequenceStart) {
			floorAnimStartIndex = i;
		}
		if (track.frame <= current_frame) {
			floorIndex = i;
		}
		if (track.frame >= current_frame && ceilIndex == -1) {
			ceilIndex = i;
		}
		if (track.frame <= sequenceEnd) {
			floorAnimEndIndex = i;
		} else {
			// end of our sequence
			break;
		}
	}
	if (ceilIndex == -1) {
		ceilIndex = header.tracks.size() - 1;
	}

	// end get floor
	if (ceilIndex < floorIndex) {
		ceilIndex = floorIndex;
		// was a problem in matrix eater, different impl, not problem here?
	}

	T floorInTan{};
	T floorOutTan{};
	T floorValue{};
	T ceilValue{};
	int floorIndexTime;
	int ceilIndexTime;

	floorValue = header.tracks[floorIndex].value;
	if (header.interpolation_type > 1) {
		floorInTan = header.tracks[floorIndex].inTan;
		floorOutTan = header.tracks[floorIndex].outTan;
	}
	ceilValue = header.tracks[ceilIndex].value;
	floorIndexTime = header.tracks[floorIndex].frame;
	ceilIndexTime = header.tracks[ceilIndex].frame;
	if (ceilIndexTime < sequenceStart) {
		return defaultValue;
	}
	if (floorIndexTime > sequenceEnd) {
		return defaultValue;
	}

	if (floorIndexTime < sequenceStart && ceilIndexTime > sequenceEnd) {
		return defaultValue;
	} else if (floorIndexTime < sequenceStart) {
		if (header.tracks[floorAnimEndIndex].frame == sequenceEnd) {
			// no "floor" frame found, but we have a ceil frame,
			// so the prev frame is a repeat of animation's end
			// placed at the beginning
			floorIndex = floorAnimEndIndex;
			floorValue = header.tracks[floorAnimEndIndex].value;
			floorIndexTime = sequenceStart;
			if (header.interpolation_type > 1) {
				floorInTan = header.tracks[floorAnimEndIndex].inTan;
				floorOutTan = header.tracks[floorAnimEndIndex].outTan;
			}
		} else {
			floorValue = defaultValue;
			floorInTan = floorOutTan = defaultValue;
			floorIndexTime = sequenceStart;
		}
	} else if (ceilIndexTime > sequenceEnd || (ceilIndexTime < current_frame && header.tracks[floorAnimEndIndex].frame < current_frame)) {
		// if we have a floor frame but the "ceil" frame is after end of sequence,
		// or our ceil frame is before our time, meaning that we're at the end of the
		// entire timeline, then we need to inject a "ceil" frame at end of sequence
		if (header.tracks[floorAnimStartIndex].frame == sequenceStart) {
			ceilValue = header.tracks[floorAnimStartIndex].value;
			ceilIndex = floorAnimStartIndex;
			ceilIndexTime = sequenceStart;
		}
		// for the else case here, Matrix Eater code says to leave it blank,
		// example model is Water Elemental's birth animation, to verify behavior
	}
	if (floorIndex == ceilIndex) {
		return floorValue;
	}
	const T ceilInTan = header.tracks[ceilIndex].inTan;
	float t = std::clamp((current_frame - floorIndexTime) / (float)(ceilIndexTime - floorIndexTime), 0.f, 1.f);

	return interpolate(floorValue, floorOutTan, ceilInTan, ceilValue, t, header.interpolation_type);

#endif
}

template glm::vec3 SkeletalModelInstance::interpolate_keyframes(mdx::TrackHeader<glm::vec3>& header, const glm::vec3& defaultValue) const;
template glm::quat SkeletalModelInstance::interpolate_keyframes(mdx::TrackHeader<glm::quat>& header, const glm::quat& defaultValue) const;