module;

#include <chrono>
#include <vector>
#include <memory>
#include <unordered_map>
#include <print>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

export module SkeletalModelInstance;

import Camera;
import Utilities;
import MathOperations;
import RenderNode;
import MDX;

// Ghostwolf mentioned this to me once, so I used it,
// as 0.75, experimentally determined as a guess at
// whatever WC3 is doing. Do more research if necessary?
#define MAGIC_RENDER_SHOW_CONSTANT 0.75

// Instead of recalculating the extents of the current sequence every frame we can keep track of it
struct CurrentKeyFrame {
	int start = -1;
	int end = 0;
	int left = 0;
	int right = 0;
};

export class SkeletalModelInstance {
  public:
	std::shared_ptr<mdx::MDX> model;

	int sequence_index = 0; // can be -1 if not animating
	int current_frame = 0;

	glm::mat4 matrix = glm::mat4(1.f);

	std::vector<CurrentKeyFrame> current_keyframes;
	std::vector<RenderNode> render_nodes;
	std::vector<glm::mat4> world_matrices;

	SkeletalModelInstance() = default;
	explicit SkeletalModelInstance(std::shared_ptr<mdx::MDX> model) : model(model) {
		size_t node_count = model->bones.size() +
							model->lights.size() +
							model->help_bones.size() +
							model->attachments.size() +
							model->emitters1.size() +
							model->emitters2.size() +
							model->ribbons.size() +
							model->event_objects.size() +
							model->collision_shapes.size() +
							model->corn_emitters.size();

		// ToDo: for each camera: add camera source node to renderNodes
		render_nodes.resize(node_count);
		world_matrices.resize(node_count);
		model->for_each_node([&](mdx::Node& node) {
			// Seen it happen with Emmitter1, is this an error in the model?
			// ToDo purge (when adding a validation layer or just crashing)
			if (node.id == -1) {
				return;
			}

			RenderNode renderNode = RenderNode(node, model->pivots[node.id]);
			if (node.parent_id != -1) {
				renderNode.parent = &render_nodes[node.parent_id];
			} else {
				renderNode.parent = nullptr;
			}

			render_nodes[node.id] = renderNode;
		});

		current_keyframes.resize(model->unique_tracks);

		for (size_t i = 0; i < model->sequences.size(); i++) {
			if (model->sequences[i].name.find("Stand") != std::string::npos || model->sequences[i].name.find("stand") != std::string::npos) {
				set_sequence(static_cast<int>(i));
				break;
			}
		}
	}

	void update_location(glm::vec3 position, glm::quat rotation, const glm::vec3& scale) {
		fromRotationTranslationScaleOrigin(rotation, position, scale, matrix, glm::vec3(0, 0, 0));
	}

	void update_location(glm::vec3 position, float angle, const glm::vec3& scale) {
		glm::quat rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));
		fromRotationTranslationScaleOrigin(rotation, position, scale, matrix, glm::vec3(0, 0, 0));
	}

	void update(double delta) {
		if (model->sequences.empty() || sequence_index == -1) {
			return;
		}

		// Advance current frame
		const mdx::Sequence& sequence = model->sequences[sequence_index];
		//if (sequence.flags & mdx::Sequence::non_looping) {
		//	current_frame = std::min<int>(current_frame + delta * 1000.0, sequence.end_frame);
		//} else {
			current_frame += delta * 1000.0;
			if (current_frame > sequence.end_frame) {
				current_frame = sequence.start_frame;
			}
		//}

		for (const auto& i : render_nodes) {
			advance_keyframes(i.node->KGTR);
			advance_keyframes(i.node->KGRT);
			advance_keyframes(i.node->KGSC);
		}

		for (const auto& i : model->animations) {
			advance_keyframes(i.KGAC);
			advance_keyframes(i.KGAO);
		}

		for (const auto& i : model->materials) {
			for (const auto& j : i.layers) {
				advance_keyframes(j.KMTA);
				// Add more when required
			}
		}

		update_nodes();
	}

	void update_nodes() {
		assert(sequence_index >= 0 && sequence_index < model->sequences.size());

		// update skeleton to position based on animation @ time
		for (auto& node : render_nodes) {
			// node.position = interpolate_keyframes(node.node->KGTR, TRANSLATION_IDENTITY);
			// node.rotation = interpolate_keyframes(node.node->KGRT, ROTATION_IDENTITY);
			// node.scale = interpolate_keyframes(node.node->KGSC, SCALE_IDENTITY);

			glm::vec3 position = interpolate_keyframes(node.node->KGTR, TRANSLATION_IDENTITY);
			glm::quat rotation = interpolate_keyframes(node.node->KGRT, ROTATION_IDENTITY);
			glm::vec3 scale = interpolate_keyframes(node.node->KGSC, SCALE_IDENTITY);

			fromRotationTranslationScaleOrigin(rotation, position, scale, world_matrices[node.node->id], node.pivot);

			if (node.node->parent_id != -1) {
				world_matrices[node.node->id] = world_matrices[node.node->parent_id] * world_matrices[node.node->id];
			}

			if (node.billboarded || node.billboardedX) {

				world_matrices[node.node->id][1][0] = 0.f;
				world_matrices[node.node->id][2][0] = 0.f;
				world_matrices[node.node->id][3][0] = 0.f;
				world_matrices[node.node->id][2][1] = 0.f;
				world_matrices[node.node->id][3][1] = 0.f;
				world_matrices[node.node->id][3][2] = 0.f;

				world_matrices[node.node->id][0][1] = 0.f;
				world_matrices[node.node->id][0][2] = 0.f;
				world_matrices[node.node->id][0][3] = 0.f;
				world_matrices[node.node->id][1][2] = 0.f;
				world_matrices[node.node->id][1][3] = 0.f;
				world_matrices[node.node->id][2][3] = 0.f;

				// Cancel the parent's rotation
				/*if (node.parent) {
					node.localRotation = node.parent->inverseWorldRotation * inverseInstanceRotation;
				} else {
					node.localRotation = inverseInstanceRotation;
				}

				node.localRotation *= camera->decomposed_rotation;*/
			}
		}
	}

	/// Sets the current sequence to sequence_index and recalculates required keyframe data
	void set_sequence(int sequence_index) {
		this->sequence_index = sequence_index;
		current_frame = model->sequences[sequence_index].start_frame;

		for (const auto& i : render_nodes) {
			calculate_sequence_extents(i.node->KGTR);
			calculate_sequence_extents(i.node->KGRT);
			calculate_sequence_extents(i.node->KGSC);
		}

		for (const auto& i : model->animations) {
			calculate_sequence_extents(i.KGAC);
			calculate_sequence_extents(i.KGAO);
		}

		for (const auto& i : model->materials) {
			for (const auto& j : i.layers) {
				calculate_sequence_extents(j.KMTA);
				// Add more when required
			}
		}
	}
	
	template <typename T>
	void calculate_sequence_extents(const mdx::TrackHeader<T>& header) {
		if (header.id == -1) {
			return;
		}

		const mdx::Sequence& sequence = model->sequences[sequence_index];
		int local_sequence_start = sequence.start_frame;
		int local_sequence_end = sequence.end_frame;

		if (header.global_sequence_ID >= 0 && model->global_sequences.size()) {
			local_sequence_start = 0;
			local_sequence_end = model->global_sequences[header.global_sequence_ID];
		}

		CurrentKeyFrame& current = current_keyframes[header.id];
		current.start = -1;
		current.right = -1;

		// Find the sequence start and end tracks, these are not always exactly at the sequence start/end
		for (int i = 0; i < header.tracks.size(); i++) {
			const mdx::Track<T>& track = header.tracks[i];

			if (track.frame > local_sequence_end) {
				break;
			}

			if (track.frame >= local_sequence_start && current.start == -1) {
				current.start = i;
			}

			current.end = i;
		}

		// Set the starting left/right track index
		if (current.start != -1) {
			current.left = current.start;

			if (current.end > current.start) {
				current.right = current.left + 1;
			} else {
				current.right = current.left;
			}
		}
	}


	template <typename T>
	void advance_keyframes(const mdx::TrackHeader<T>& header) {
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

			// No need for interpolation if current_frame is exactly on a track
			if (header.tracks[current.right].frame == local_current_frame) {
				current.left = current.right;
			}
		}

		// The first/last tracks are not always exactly at the sequence start/end
		const bool past_end = header.tracks[current.end].frame < local_current_frame;
		const bool before_start = header.tracks[current.start].frame > local_current_frame;
		if (past_end || before_start) {
			current.left = current.end;
			current.right = current.start;
		}
	}

	// Returns RGB instead of BGR as Blizzard used internally
	glm::vec3 get_geoset_animation_color(const mdx::GeosetAnimation& animation) const {
		return interpolate_keyframes<glm::vec3>(animation.KGAC, animation.color.bgr).bgr;
	}

	float get_geoset_animation_visiblity(const mdx::GeosetAnimation& animation) const {
		return interpolate_keyframes(animation.KGAO, animation.alpha);
	}

	float get_layer_visiblity(const mdx::Layer& layer) const {
		return interpolate_keyframes(layer.KMTA, layer.alpha);
	}

	template <typename T>
	T interpolate_keyframes(const mdx::TrackHeader<T>& header, const T& default_value) const {
		if (header.id == -1) {
			return default_value;
		}

		const CurrentKeyFrame& current = current_keyframes[header.id];
		const mdx::Sequence& sequence = model->sequences[sequence_index];

		int local_current_frame = current_frame;
		int local_sequence_start = sequence.start_frame;
		int local_sequence_end = sequence.end_frame;

		if (header.global_sequence_ID >= 0 && model->global_sequences.size()) {
			local_sequence_start = 0;
			local_sequence_end = model->global_sequences[header.global_sequence_ID];
			if (local_sequence_end == 0) {
				local_current_frame = 0;
			} else {
				local_current_frame = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % local_sequence_end;
			}
		}

		// If there are no tracks in sequence
		if (current.start == -1) {
			return default_value;
		}

		// If there is only 1 track
		if (current.start == current.end) {
			return header.tracks[current.left].value;
		}

		const T ceil_in_tan = header.tracks[current.right].inTan;
		const T floor_out_tan = header.tracks[current.left].outTan;

		int floor_time = header.tracks[current.left].frame;
		const int ceil_time = header.tracks[current.right].frame;
		const T floor_value = header.tracks[current.left].value;
		const T ceil_value = header.tracks[current.right].value;

		// This is the implementation that correctly handles missing start/end frames.
		// The game and WE however have a buggy implementation which is the one we end up using for compatibility
		// float t;
		// if (ceil_time - floor_time < 0) {
		//	int duration = (local_sequence_end - floor_time) + (ceil_time - local_sequence_start);

		//	if (local_current_frame > floor_time) {
		//		t = (local_current_frame - floor_time) / (float)duration;
		//	} else {
		//		t = (local_current_frame - local_sequence_start + (local_sequence_end - floor_time)) / (float)duration;
		//	}
		//} else {
		//	t = (local_current_frame - floor_time) / static_cast<float>(ceil_time - floor_time);
		//}

		// The (incorrect) implementation both the game and WE use
		int time_between_frames = ceil_time - floor_time;
		if (time_between_frames < 0) {
			time_between_frames += (local_sequence_end - local_sequence_start);
			if (local_current_frame < floor_time) {
				floor_time = ceil_time;
			}
		}
		const float t = time_between_frames == 0 ? 0.f : ((local_current_frame - floor_time) / static_cast<float>(time_between_frames));

		return interpolate(floor_value, floor_out_tan, ceil_in_tan, ceil_value, t, header.interpolation_type);
	}
};