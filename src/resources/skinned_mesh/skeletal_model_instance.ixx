module;

#include <cassert>
#include <chrono>

export module SkeletalModelInstance;

import std;
import Camera;
import Utilities;
import MathOperations;
import RenderNode;
import MDX;
import ParticleEmitter2Simulation;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;

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

// Recognized tokens for sequence selection. Names with substrings not in this set are ignored
// when tokenizing a sequence name; the request side substitutes "stand" for unrecognized substrings.
static constexpr std::array<std::string_view, 64> sequence_tokens = {
	"alternate", "alternateex", "attack",	 "berserk", "birth",   "chain",	   "channel",  "cinematic", "complete", "critical", "death",
	"decay",	 "defend",		"dissipate", "drain",	"eattree", "entangle", "fast",	   "fifth",		"fill",		"fire",		"first",
	"five",		 "flail",		"flesh",	 "four",	"fourth",  "gold",	   "hit",	   "large",		"left",		"light",	"looping",
	"lumber",	 "medium",		"moderate",	 "morph",	"off",	   "one",	   "portrait", "puke",		"ready",	"right",	"second",
	"severe",	 "sleep",		"slam",		 "small",	"spiked",  "spell",	   "spin",	   "stand",		"swim",		"talk",		"third",
	"three",	 "throw",		"two",		 "turn",	"upgrade", "victory",  "walk",	   "work",		"wounded",
};

static bool is_recognized_sequence_token(const std::string_view token) {
	return std::ranges::find(sequence_tokens, token) != sequence_tokens.end();
}

// Splits `name` on whitespace (lowercased). Unrecognized substrings are dropped, or replaced by "stand"
// when `replace_unrecognized` is true (used for the request side, per the selection spec).
static std::vector<std::string> tokenize_sequence_name(const std::string_view name, const bool replace_unrecognized) {
	std::vector<std::string> tokens;
	size_t i = 0;
	while (i < name.size()) {
		while (i < name.size() && std::isspace(static_cast<unsigned char>(name[i]))) {
			i++;
		}
		const size_t start = i;
		while (i < name.size() && !std::isspace(static_cast<unsigned char>(name[i]))) {
			i++;
		}
		if (start == i) {
			break;
		}
		std::string token(name.substr(start, i - start));
		std::ranges::transform(token, token.begin(), [](unsigned char c) {
			return std::tolower(c);
		});
		if (is_recognized_sequence_token(token)) {
			tokens.push_back(std::move(token));
		} else if (replace_unrecognized) {
			tokens.emplace_back("stand");
		}
	}
	return tokens;
}

export class SkeletalModelInstance {
  public:
	std::shared_ptr<mdx::MDX> model;

	// Todo, with validate() we ensure all MDXs have at least one sequence, so this could be a size_t
	int sequence_index = 0; // can be -1 if not animating
	// Todo can this be negative?
	int current_frame = 0;

	// Global sequences normally animate against wall-clock time. When this is >= 0 it overrides that
	// clock with a fixed time (ms), so global-sequence tracks can be evaluated deterministically.
	// calculate_animated_extents uses this to sweep global motion without nondeterministic results.
	int64_t global_sequence_time = -1;

	glm::mat4 matrix = glm::mat4(1.f);

	std::vector<CurrentKeyFrame> current_keyframes;
	std::vector<RenderNode> render_nodes;
	std::vector<glm::mat4> world_matrices;

	ParticleEmitter2Simulation particles;
	bool sequence_just_started = true;

	std::vector<std::string> required_animation_names;

	SkeletalModelInstance() = default;

	explicit SkeletalModelInstance(const std::shared_ptr<mdx::MDX>& model, std::vector<std::string>&& required_animation_names = {}) :
		model(model),
		required_animation_names(required_animation_names) {
		const size_t node_count = model->bones.size() + model->lights.size() + model->help_bones.size() + model->attachments.size()
			+ model->emitters1.size() + model->emitters2.size() + model->ribbons.size() + model->event_objects.size()
			+ model->collision_shapes.size() + model->corn_emitters.size();

		// ToDo: for each camera: add camera source node to renderNodes
		render_nodes.resize(node_count);
		world_matrices.resize(node_count);
		model->for_each_node([&](const mdx::Node& node) {
			render_nodes[node.id] = RenderNode(node, model->pivots[node.id]);
		});

		current_keyframes.resize(model->unique_tracks);

		particles.init(*model);

		set_sequence("stand");
	}

	void update_location(const glm::vec3 position, const glm::quat& rotation, const glm::vec3& scale) {
		from_rotation_translation_scale_origin(rotation, position, scale, matrix, glm::vec3(0, 0, 0));
	}

	void update_location(const glm::vec3 position, const float angle, const glm::vec3& scale) {
		const glm::quat rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));
		from_rotation_translation_scale_origin(rotation, position, scale, matrix, glm::vec3(0, 0, 0));
	}

	void update(const double delta) {
		if (model->sequences.empty() || sequence_index == -1) {
			return;
		}

		// Advance current frame
		const mdx::Sequence& sequence = model->sequences[sequence_index];
		const int frame_before_advance = current_frame;
		//if (sequence.flags & mdx::Sequence::non_looping) {
		//	current_frame = std::min<int>(current_frame + delta * 1000.0, sequence.end_frame);
		//} else {
		current_frame += delta * 1000.0;
		if (current_frame > sequence.end_frame) {
			current_frame = sequence.start_frame;
		}
		//}

		const bool sequence_wrapped = sequence_just_started || (current_frame < frame_before_advance);
		sequence_just_started = false;

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

		for (const auto& i : model->emitters2) {
			advance_keyframes(i.KP2S);
			advance_keyframes(i.KP2R);
			advance_keyframes(i.KP2L);
			advance_keyframes(i.KP2G);
			advance_keyframes(i.KP2E);
			advance_keyframes(i.KP2N);
			advance_keyframes(i.KP2W);
			advance_keyframes(i.KP2V);
		}

		update_nodes();
		update_particle_emitters2(delta, sequence_wrapped);
	}

	void update_particle_emitters2(const double delta, const bool sequence_wrapped) {
		for (size_t i = 0; i < model->emitters2.size(); ++i) {
			const mdx::ParticleEmitter2& e = model->emitters2[i];

			ParticleEmitter2Simulation::EmitterFrameParams p {};
			p.emission_rate = interpolate_keyframes(e.KP2E, e.emission_rate);
			p.speed = interpolate_keyframes(e.KP2S, e.speed);
			p.speed_variation = interpolate_keyframes(e.KP2R, e.speed_variation);
			p.latitude = glm::radians(interpolate_keyframes(e.KP2L, e.latitude));
			p.gravity = interpolate_keyframes(e.KP2G, e.gravity);
			p.width = interpolate_keyframes(e.KP2W, e.width);
			p.length = interpolate_keyframes(e.KP2N, e.length);
			p.visibility = interpolate_keyframes(e.KP2V, 1.0f);
			// world_matrices[node.id] is built in skinning convention (no pivot offset
			// when the node is at rest), so append the pivot translation to recover
			// the emitter's actual world position.
			p.world_matrix = world_matrices[e.node.id] * glm::translate(glm::mat4(1.f), model->pivots[e.node.id]);
			p.sequence_just_wrapped = sequence_wrapped;

			particles.update_emitter(i, delta, e, p);
		}
	}

	// apply_billboard rotates billboarded nodes to face the camera. Pass false to get a stable,
	// camera-independent pose (used by calculate_animated_extents, which must not depend on the
	// global camera and wants geometry bounds, not sprite orientation).
	void update_nodes(const bool apply_billboard = true) {
		assert(sequence_index >= 0 && sequence_index < model->sequences.size());

		const glm::mat3 inverse_model_rotation = glm::transpose(
			glm::mat3 {
				glm::normalize(glm::vec3(matrix[0])),
				glm::normalize(glm::vec3(matrix[1])),
				glm::normalize(glm::vec3(matrix[2])),
			}
		);

		// Todo, node->parent_id can be higher than their current id. No ordering exists
		// So in the loop below the parent_matrix might be empty for the first frame and an animation might be one frame behind
		for (const auto& node : render_nodes) {
			const glm::vec3 position = interpolate_keyframes(node.node->KGTR, TRANSLATION_IDENTITY);
			const glm::quat rotation = interpolate_keyframes(node.node->KGRT, ROTATION_IDENTITY);
			const glm::vec3 scale = interpolate_keyframes(node.node->KGSC, SCALE_IDENTITY);

			glm::quat final_rotation = rotation;
			if (apply_billboard && node.billboarded) {
				const glm::mat3 cam_world = inverse_model_rotation * glm::mat3(camera.view_inverse);
				const glm::mat3 inverse_camera_world = glm::mat3(cam_world[2], cam_world[0], cam_world[1]);

				if (node.node->parent_id != -1) {
					// Extract and invert parent rotation to keep billboard effect
					const glm::mat4& parent_matrix = world_matrices[node.node->parent_id];
					const glm::vec3 parent_scale = glm::vec3(
						glm::length(glm::vec3(parent_matrix[0])),
						glm::length(glm::vec3(parent_matrix[1])),
						glm::length(glm::vec3(parent_matrix[2]))
					);
					const glm::mat3 parent_rotation = glm::mat3(
						glm::vec3(parent_matrix[0]) / parent_scale.x,
						glm::vec3(parent_matrix[1]) / parent_scale.y,
						glm::vec3(parent_matrix[2]) / parent_scale.z
					);
					final_rotation = glm::quat_cast(glm::transpose(parent_rotation)) * glm::quat_cast(inverse_camera_world);
				} else {
					final_rotation = glm::quat_cast(inverse_camera_world);
				}
			}

			from_rotation_translation_scale_origin(final_rotation, position, scale, world_matrices[node.node->id], node.pivot);

			if (node.node->parent_id != -1) {
				world_matrices[node.node->id] = world_matrices[node.node->parent_id] * world_matrices[node.node->id];
			}
		}
	}

	// Sets the current sequence to sequence_index and recalculates required keyframe data
	void set_sequence(const int sequence_index) {
		this->sequence_index = sequence_index;
		current_frame = model->sequences[sequence_index].start_frame;
		sequence_just_started = true;

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

		for (const auto& i : model->emitters2) {
			calculate_sequence_extents(i.KP2S);
			calculate_sequence_extents(i.KP2R);
			calculate_sequence_extents(i.KP2L);
			calculate_sequence_extents(i.KP2G);
			calculate_sequence_extents(i.KP2E);
			calculate_sequence_extents(i.KP2N);
			calculate_sequence_extents(i.KP2W);
			calculate_sequence_extents(i.KP2V);
		}
	}

	// True if `name` contains at least one WC3-recognized animation token. Lets callers filter
	// out sequences like "nothing" that the set_sequence tiebreaker may otherwise pick over a
	// recognized-but-mismatched name (e.g. "Birth" when asking for "stand").
	static bool sequence_name_has_recognized_token(const std::string_view name) {
		return !tokenize_sequence_name(name, false).empty();
	}

	// True if the sequence has the uninitialized-extent sentinel (min/max swapped to +/-FLT_MAX).
	// Spell models like FlameStrike ship empty "stand"/"death" placeholders that match by name but
	// have no animated content — previews should fall back to "birth". Bounds_radius alone isn't
	// reliable: ChimaeraAcidMissile has sentinel extents AND a non-zero bounds_radius copied from
	// the geoset.
	static bool sequence_has_empty_extent(const mdx::Sequence& sequence) {
		return sequence.extent.minimum.x > sequence.extent.maximum.x;
	}

	// Adjust the skeleton's current sequence to one suited for a static/looping preview. The
	// constructor's `set_sequence("stand")` works for unit models but for spell effects with no
	// "stand" sequence its random tiebreaker can land on a "death" sequence whose Visibility
	// track holds the emitters at 0 which is an empty thumbnail. We want to keep a suitable stand and
	// otherwise prefer a Birth-named sequence, otherwise any suitable sequence, otherwise leave it.
	static void pick_preview_sequence(SkeletalModelInstance& skeleton, const mdx::MDX& mdx) {
		auto suitable = [&](size_t i) {
			const auto& s = mdx.sequences[i];
			return sequence_name_has_recognized_token(s.name) && !sequence_has_empty_extent(s);
		};
		auto lower_name = [](const mdx::Sequence& s) {
			std::string out = s.name;
			std::ranges::transform(out, out.begin(), [](unsigned char c) {
				return std::tolower(c);
			});
			return out;
		};

		const int current = skeleton.sequence_index;
		const bool current_valid = current >= 0 && current < static_cast<int>(mdx.sequences.size());

		if (current_valid && suitable(current) && lower_name(mdx.sequences[current]).contains("stand")) {
			return;
		}

		for (size_t i = 0; i < mdx.sequences.size(); ++i) {
			if (suitable(i) && lower_name(mdx.sequences[i]).contains("birth")) {
				skeleton.set_sequence(static_cast<int>(i));
				return;
			}
		}

		if (current_valid && suitable(current)) {
			return;
		}

		for (size_t i = 0; i < mdx.sequences.size(); ++i) {
			if (suitable(i)) {
				skeleton.set_sequence(static_cast<int>(i));
				return;
			}
		}
	}

	/// Set sequence by name, matching the WC3 (Reforged) token-based selection rules.
	///
	/// The request is split into whitespace-delimited substrings; unrecognized substrings are
	/// replaced by "stand". Each animation in the model is tokenized the same way, but
	/// unrecognized substrings are dropped. For every candidate we count tokens that match the
	/// request (order-insensitive) and tokens that do not. The best match is the animation with
	/// the most matches; ties are broken by fewest non-matches. Among remaining ties one is
	/// picked at random, weighted by the per-sequence Rarity (uniform when all rarities are 0).
	///
	/// Special case: if the request has at least two recognized tokens and the first is
	/// "cinematic", we first try an exact full-string (case-insensitive) name match before
	/// falling back to the standard tokenized selection.
	void set_sequence(const std::string& sequence_name) {
		const auto& sequences = model->sequences;
		if (sequences.empty()) {
			return;
		}

		// Cinematic special-case: arg with 2+ recognized tokens whose first recognized token is
		// "cinematic" attempts a full-string match before falling back to tokenized selection.
		const auto recognized_request = tokenize_sequence_name(sequence_name, false);
		if (recognized_request.size() >= 2 && recognized_request.front() == "cinematic") {
			const std::string lower_request = to_lowercase_copy(sequence_name);
			for (size_t i = 0; i < sequences.size(); i++) {
				if (to_lowercase_copy(sequences[i].name) == lower_request) {
					set_sequence(static_cast<int>(i));
					return;
				}
			}
		}

		std::vector<std::string> request_tokens = tokenize_sequence_name(sequence_name, true);
		if (request_tokens.empty()) {
			request_tokens.emplace_back("stand");
		}

		struct Candidate {
			int index;
			int matches;
			int mismatches;
			float rarity;
		};

		std::vector<Candidate> candidates;
		candidates.reserve(sequences.size());

		for (size_t i = 0; i < sequences.size(); i++) {
			const std::string lower_name = to_lowercase_copy(sequences[i].name);

			bool meets_required = true;
			for (const auto& req : required_animation_names) {
				if (!lower_name.contains(req)) {
					meets_required = false;
					break;
				}
			}
			if (!meets_required) {
				continue;
			}

			const auto anim_tokens = tokenize_sequence_name(lower_name, false);

			int matches = 0;
			int mismatches = 0;
			for (const auto& tok : anim_tokens) {
				if (std::ranges::find(request_tokens, tok) != request_tokens.end()) {
					matches++;
				} else {
					mismatches++;
				}
			}

			candidates.push_back({static_cast<int>(i), matches, mismatches, sequences[i].rarity});
		}

		if (candidates.empty()) {
			return;
		}

		int best_matches = -1;
		int best_mismatches = std::numeric_limits<int>::max();
		for (const auto& c : candidates) {
			if (c.matches > best_matches || (c.matches == best_matches && c.mismatches < best_mismatches)) {
				best_matches = c.matches;
				best_mismatches = c.mismatches;
			}
		}

		std::vector<Candidate> winners;
		for (const auto& c : candidates) {
			if (c.matches == best_matches && c.mismatches == best_mismatches) {
				winners.push_back(c);
			}
		}

		int chosen = winners.front().index;
		if (winners.size() > 1) {
			static thread_local std::mt19937 rng {std::random_device {}()};

			float total_weight = 0.f;
			for (const auto& w : winners) {
				total_weight += w.rarity;
			}

			if (total_weight > 0.f) {
				std::uniform_real_distribution<float> dist(0.f, total_weight);
				float roll = dist(rng);
				for (const auto& w : winners) {
					roll -= w.rarity;
					if (roll <= 0.f) {
						chosen = w.index;
						break;
					}
				}
			} else {
				std::uniform_int_distribution<size_t> dist(0, winners.size() - 1);
				chosen = winners[dist(rng)].index;
			}
		}

		set_sequence(chosen);
	}

	template<typename T>
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

	// Time (ms) used to evaluate global-sequence tracks: wall-clock normally, or the fixed override
	// global_sequence_time when set (used for deterministic extent sampling).
	int64_t global_sequence_now() const {
		if (global_sequence_time >= 0) {
			return global_sequence_time;
		}
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	template<typename T>
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
				local_current_frame = global_sequence_now() % local_sequence_end;
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
			current.right += 1;

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
		const auto color = interpolate_keyframes<glm::vec3>(animation.KGAC, animation.color);
		return {color.b, color.g, color.r};
	}

	float get_geoset_animation_visiblity(const mdx::GeosetAnimation& animation) const {
		return interpolate_keyframes(animation.KGAO, animation.alpha);
	}

	float get_layer_visiblity(const mdx::Layer& layer) const {
		return interpolate_keyframes(layer.KMTA, layer.alpha);
	}

	template<typename T>
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
				local_current_frame = global_sequence_now() % local_sequence_end;
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

		return interpolate(floor_value, floor_out_tan, ceil_in_tan, ceil_value, t, static_cast<int>(header.interpolation_type));
	}
};

// Number of evenly spaced interior samples inserted between consecutive keyframes. Linear tracks
// only need the keyframes themselves, but hermite/bezier curves can overshoot between them.
static constexpr int extent_subdivisions = 4;
// Safety cap on samples per sequence so a densely keyframed long animation can't explode the cost.
static constexpr size_t max_extent_samples = 256;

// Builds the sorted, deduped list of frames to sample within [start_frame, end_frame]: every node
// translation/rotation/scale keyframe in range, the sequence start/end, and a few interior samples
// between consecutive candidates. Only node tracks move geometry, so material/emitter/geoset-anim
// tracks are ignored here.
static std::vector<int> build_sample_frames(mdx::MDX& model, const mdx::Sequence& sequence) {
	const int start = static_cast<int>(sequence.start_frame);
	const int end = static_cast<int>(sequence.end_frame);

	std::vector<int> frames;
	frames.push_back(start);
	frames.push_back(end);

	const auto add_track_frames = [&](const auto& header) {
		for (const auto& track : header.tracks) {
			if (track.frame >= start && track.frame <= end) {
				frames.push_back(track.frame);
			}
		}
	};

	model.for_each_node([&](const mdx::Node& node) {
		add_track_frames(node.KGTR);
		add_track_frames(node.KGRT);
		add_track_frames(node.KGSC);
	});

	std::ranges::sort(frames);
	frames.erase(std::ranges::unique(frames).begin(), frames.end());

	// Subdivide the gaps between consecutive keyframes to catch curve overshoot.
	std::vector<int> samples;
	for (size_t i = 0; i + 1 < frames.size(); i++) {
		samples.push_back(frames[i]);
		const int gap = frames[i + 1] - frames[i];
		for (int j = 1; j <= extent_subdivisions; j++) {
			const int interior = frames[i] + gap * j / (extent_subdivisions + 1);
			if (interior > frames[i] && interior < frames[i + 1]) {
				samples.push_back(interior);
			}
		}
	}
	samples.push_back(frames.back());

	std::ranges::sort(samples);
	samples.erase(std::ranges::unique(samples).begin(), samples.end());

	if (samples.size() > max_extent_samples) {
		std::vector<int> trimmed;
		trimmed.reserve(max_extent_samples);
		for (size_t i = 0; i < max_extent_samples; i++) {
			trimmed.push_back(samples[i * (samples.size() - 1) / (max_extent_samples - 1)]);
		}
		trimmed.erase(std::ranges::unique(trimmed).begin(), trimmed.end());
		samples = std::move(trimmed);
	}

	return samples;
}

// Safety cap on global-sequence phase samples so the product with the per-sequence frame samples
// stays bounded.
static constexpr size_t max_global_samples = 64;

// Builds deterministic sample times (ms) for global-sequence-driven node tracks. Global sequences
// animate against wall-clock time, so without pinning the phase the extents would differ on every
// call. We sweep each global sequence's period (anchored on the keyframes of node tracks bound to it,
// plus interior subdivisions) so the captured bounds cover the full global motion. Returns {0} when
// no node track is driven by a global sequence (single deterministic pass, no extra cost).
static std::vector<int64_t> build_global_sample_times(mdx::MDX& model) {
	if (model.global_sequences.empty()) {
		return {0};
	}

	std::vector<int64_t> times;
	const auto add_track = [&](const auto& header) {
		if (header.global_sequence_ID < 0 || header.global_sequence_ID >= static_cast<int>(model.global_sequences.size())) {
			return;
		}
		const int period = model.global_sequences[header.global_sequence_ID];
		if (period <= 0) {
			return;
		}
		times.push_back(0);
		times.push_back(period);
		for (const auto& track : header.tracks) {
			if (track.frame >= 0 && track.frame <= period) {
				times.push_back(track.frame);
			}
		}
	};

	model.for_each_node([&](const mdx::Node& node) {
		add_track(node.KGTR);
		add_track(node.KGRT);
		add_track(node.KGSC);
	});

	if (times.empty()) {
		return {0};
	}

	std::ranges::sort(times);
	times.erase(std::ranges::unique(times).begin(), times.end());

	// Subdivide the gaps between consecutive keyframes to catch curve overshoot, same as the local
	// frame sampler.
	std::vector<int64_t> samples;
	for (size_t i = 0; i + 1 < times.size(); i++) {
		samples.push_back(times[i]);
		const int64_t gap = times[i + 1] - times[i];
		for (int j = 1; j <= extent_subdivisions; j++) {
			const int64_t interior = times[i] + gap * j / (extent_subdivisions + 1);
			if (interior > times[i] && interior < times[i + 1]) {
				samples.push_back(interior);
			}
		}
	}
	samples.push_back(times.back());

	std::ranges::sort(samples);
	samples.erase(std::ranges::unique(samples).begin(), samples.end());

	if (samples.size() > max_global_samples) {
		std::vector<int64_t> trimmed;
		trimmed.reserve(max_global_samples);
		for (size_t i = 0; i < max_global_samples; i++) {
			trimmed.push_back(samples[i * (samples.size() - 1) / (max_global_samples - 1)]);
		}
		trimmed.erase(std::ranges::unique(trimmed).begin(), trimmed.end());
		samples = std::move(trimmed);
	}

	return samples;
}

// Computes per-sequence and overall model extents that account for skeletal animation
export void calculate_animated_extents(const std::shared_ptr<mdx::MDX>& model) {
	// Seeds the rest-pose geoset/model extents and the particle emitter bounds.
	model->calculate_extents();

	if (model->sequences.empty() || model->bones.empty() || model->geosets.empty()) {
		return;
	}

	for (auto& geoset : model->geosets) {
		geoset.sequence_extents.resize(model->sequences.size());
	}

	// Per-geoset skin weights as (bone index, weight) pairs per vertex. HD geosets store this in
	// geoset.skin directly; SD geosets derive it from their matrix groups.
	std::vector<std::vector<glm::u8vec4>> geoset_skins(model->geosets.size());
	for (size_t g = 0; g < model->geosets.size(); g++) {
		if (model->geosets[g].skin.empty()) {
			geoset_skins[g] = mdx::MDX::matrix_groups_as_skin_weights(model->geosets[g]);
		}
	}

	// Geoset might be hidden
	std::vector<const mdx::GeosetAnimation*> geoset_anim(model->geosets.size(), nullptr);
	for (const auto& animation : model->animations) {
		if (animation.geoset_id < model->geosets.size()) {
			geoset_anim[animation.geoset_id] = &animation;
		}
	}

	SkeletalModelInstance instance(model);

	// Global sequences animate against wall-clock time; pin them to a deterministic set of phases so
	// the extents are reproducible and cover the full global motion.
	const std::vector<int64_t> global_times = build_global_sample_times(*model);

	// Keep the emitter contribution that calculate_extents() folded into model->extent.
	mdx::Extent model_ext = model->extent;

	for (size_t s = 0; s < model->sequences.size(); s++) {
		instance.set_sequence(static_cast<int>(s));
		instance.global_sequence_time = global_times.front();
		// Warm up parent matrices once (a node's parent can have a higher id, so the first pose may
		// otherwise read stale parents).
		instance.update_nodes(false);

		// Seed from the rest pose so a sequence with no node motion still gets valid bounds.
		mdx::Extent seq_ext;
		seq_ext.minimum = glm::vec3(std::numeric_limits<float>::max());
		seq_ext.maximum = glm::vec3(std::numeric_limits<float>::lowest());

		std::vector<glm::vec3> geoset_min(model->geosets.size(), glm::vec3(std::numeric_limits<float>::max()));
		std::vector<glm::vec3> geoset_max(model->geosets.size(), glm::vec3(std::numeric_limits<float>::lowest()));

		const std::vector<int> frames = build_sample_frames(*model, model->sequences[s]);

		// Outer loop over global-sequence phases, inner over local sequence frames. For models without
		// any global-sequence node tracks global_times is {0}, so this is a single deterministic pass.
		for (const int64_t global_time : global_times) {
			instance.global_sequence_time = global_time;

			for (const int frame : frames) {
				instance.current_frame = frame;
				for (const auto& node : instance.render_nodes) {
					instance.advance_keyframes(node.node->KGTR);
					instance.advance_keyframes(node.node->KGRT);
					instance.advance_keyframes(node.node->KGSC);
				}
				for (const auto& animation : model->animations) {
					instance.advance_keyframes(animation.KGAO);
				}
				instance.update_nodes(false);

				for (size_t g = 0; g < model->geosets.size(); g++) {
					// Skip geosets that are (near) invisible this frame; they don't contribute to bounds.
					if (geoset_anim[g] && instance.get_geoset_animation_visiblity(*geoset_anim[g]) < 0.01f) {
						continue;
					}

					const mdx::Geoset& geoset = model->geosets[g];
					const bool hd = !geoset.skin.empty();
					const std::vector<glm::u8vec4>& sd_skin = geoset_skins[g];

					for (size_t v = 0; v < geoset.vertices.size(); v++) {
						glm::uvec4 indices(0);
						glm::vec4 weights(0.f);
						if (hd) {
							const size_t base = v * 8;
							for (int j = 0; j < 4; j++) {
								indices[j] = geoset.skin[base + j];
								weights[j] = geoset.skin[base + 4 + j] / 255.f;
							}
						} else {
							indices = sd_skin[v * 2];
							const glm::u8vec4 w = sd_skin[v * 2 + 1];
							weights = glm::vec4(w.x, w.y, w.z, w.w) / 255.f;
						}

						glm::mat4 skin_matrix(0.f);
						for (int j = 0; j < 4; j++) {
							if (weights[j] > 0.f) {
								skin_matrix += instance.world_matrices[indices[j]] * weights[j];
							}
						}

						const glm::vec3 pos = glm::vec3(skin_matrix * glm::vec4(geoset.vertices[v], 1.f));
						geoset_min[g] = glm::min(geoset_min[g], pos);
						geoset_max[g] = glm::max(geoset_max[g], pos);
					}
				}
			}
		}

		for (size_t g = 0; g < model->geosets.size(); g++) {
			// Skip empty geosets and those that never accumulated a vertex this sequence (e.g. hidden
			// the entire time); their sequence extent stays zero-initialized so it adds nothing.
			if (model->geosets[g].vertices.empty() || geoset_min[g].x > geoset_max[g].x) {
				continue;
			}
			mdx::Extent& ge = model->geosets[g].sequence_extents[s];
			ge.minimum = geoset_min[g];
			ge.maximum = geoset_max[g];
			ge.bounds_radius = glm::length((ge.maximum - ge.minimum) * 0.5f);

			seq_ext.minimum = glm::min(seq_ext.minimum, ge.minimum);
			seq_ext.maximum = glm::max(seq_ext.maximum, ge.maximum);
		}

		// No geoset was ever visible this whole sequence: leave an empty (sentinel) extent and don't
		// let it expand the model bounds.
		if (seq_ext.minimum.x > seq_ext.maximum.x) {
			seq_ext.bounds_radius = 0.f;
			model->sequences[s].extent = seq_ext;
			continue;
		}

		seq_ext.bounds_radius = glm::length((seq_ext.maximum - seq_ext.minimum) * 0.5f);
		model->sequences[s].extent = seq_ext;

		model_ext.minimum = glm::min(model_ext.minimum, seq_ext.minimum);
		model_ext.maximum = glm::max(model_ext.maximum, seq_ext.maximum);
	}

	// The bounding sphere is centered on the AABB, so its radius can only be derived from the final
	// merged AABB — max-merging per-sequence radii (each measured from its own center) is invalid.
	model_ext.bounds_radius = glm::length((model_ext.maximum - model_ext.minimum) * 0.5f);
	model->extent = model_ext;
}
