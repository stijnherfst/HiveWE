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

	for (auto& i : render_nodes) {
		calculate_sequence_extents(i.node.KGTR);
		calculate_sequence_extents(i.node.KGRT);
		calculate_sequence_extents(i.node.KGSC);
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
		advance_keyframes(i.node.KGTR);
		advance_keyframes(i.node.KGRT);
		advance_keyframes(i.node.KGSC);
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
		node.localLocation = interpolate_keyframes(node.node.KGTR, TRANSLATION_IDENTITY);
		node.localRotation = interpolate_keyframes(node.node.KGRT, ROTATION_IDENTITY);
		node.localScale = interpolate_keyframes(node.node.KGSC, SCALE_IDENTITY);

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

void SkeletalModelInstance::set_sequence(int sequence_index) {
	
}

template <typename T>
void SkeletalModelInstance::calculate_sequence_extents(mdx::TrackHeader<T>& header) {
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

	header.current.start = -1;

	for (int i = 0; i < header.tracks.size(); i++) {
		const mdx::Track<T>& track = header.tracks[i];
		if (track.frame >= local_sequence_start && header.current.start == -1) {
			header.current.start = i;
		}

		if (track.frame <= local_current_frame) {
			header.current.left = i;
		}

		if (track.frame <= local_sequence_end) {
			header.current.end = i;
		} else {
			break;
		}
	}

	header.current.right = std::min(header.current.left + 1, header.current.end);
}

template void SkeletalModelInstance::calculate_sequence_extents(mdx::TrackHeader<glm::vec3>& header);
template void SkeletalModelInstance::calculate_sequence_extents(mdx::TrackHeader<glm::quat>& header);

template <typename T>
void SkeletalModelInstance::advance_keyframes(mdx::TrackHeader<T>& header) {
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
	if (header.current.start == -1) {
		return;
	}

	// If there is only 1 track
	if (header.current.start == header.current.end) {
		return;
	}

	// Detect if we looped
	if (header.tracks[header.current.left].frame > local_current_frame) {
		header.current.left = header.current.start;
		header.current.right = header.current.start + 1;
	}

	// Scan till we find two tracks
	while (header.tracks[header.current.right].frame < local_current_frame) {
		header.current.left = header.current.right;
		header.current.right++;

		// Reached last keyframe
		if (header.current.right > header.current.end) {
			break;
		}
	}

	const bool past_end = header.tracks[header.current.end].frame < local_current_frame;
	const bool before_start = header.tracks[header.current.start].frame > local_current_frame;
	if (past_end || before_start) {
		header.current.left = header.current.end;
		header.current.right = header.current.start;
	}
}

template void SkeletalModelInstance::advance_keyframes(mdx::TrackHeader<glm::vec3>& header);
template void SkeletalModelInstance::advance_keyframes(mdx::TrackHeader<glm::quat>& header);

glm::vec3 SkeletalModelInstance::get_geoset_animation_color(mdx::GeosetAnimation& animation) {
	//return interpolate_keyframes(animation.KGAC, animation.color);
	return animation.color;
}

float SkeletalModelInstance::get_geoset_animation_visiblity(mdx::GeosetAnimation& animation) {
	//return interpolate_keyframes(animation.KGAO, animation.alpha);
	return animation.alpha;
}

float SkeletalModelInstance::get_layer_visiblity(mdx::Layer& layer) {
	//return interpolate_keyframes(layer.KMTA, layer.alpha);
	return layer.alpha;
}

template <typename T>
T SkeletalModelInstance::interpolate_keyframes(mdx::TrackHeader<T>& header, const T& defaultValue) {
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
	if (header.current.start == -1) {
		return defaultValue;
	}

	// If there is only 1 track
	if (header.current.start == header.current.end) {
		return header.tracks[header.current.left].value;
	}

	const T ceilInTan = header.tracks[header.current.right].inTan;
	const T floorOutTan = header.tracks[header.current.left].outTan;

	int floorTime = header.tracks[header.current.left].frame;
	int ceilTime = header.tracks[header.current.right].frame;
	T floorValue = header.tracks[header.current.left].value;
	T ceilValue = header.tracks[header.current.right].value;

	// Wrong when wrapping around sequence end/start 
	float tt = (local_current_frame - floorTime) / static_cast<float>(ceilTime - floorTime);

	//int ttt = (sequence.end_frame - floorTime + ceilTime) / current_frame - floorTime;

	float t = std::clamp(tt, 0.f, 1.f);

	return interpolate(floorValue, floorOutTan, ceilInTan, ceilValue, t, header.interpolation_type);
}

template glm::vec3 SkeletalModelInstance::interpolate_keyframes(mdx::TrackHeader<glm::vec3>& header, const glm::vec3& defaultValue);
template glm::quat SkeletalModelInstance::interpolate_keyframes(mdx::TrackHeader<glm::quat>& header, const glm::quat& defaultValue);