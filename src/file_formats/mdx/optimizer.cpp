module;

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

module MDX;

import std;
import MathOperations;
import <glm/glm.hpp>;
import <glm/gtc/quaternion.hpp>;

namespace mdx {
	/// Reduces the number of keyframes in a track by decimating the curve
	/// Uses the Douglas–Peucker algorithm
	/// Assumes all tracks are contained in a sequence
	template <typename T>
	void reduce_track(TrackHeader<T>& header, std::vector<Sequence>& sequences, float max_error_sq) {
		if (header.tracks.size() <= 2) {
			return;
		}

		size_t write_index = 1; // Always keep the first keyframe
		size_t anchor_index = 0;

		size_t current_sequence = 0;
		for (size_t i = 1; i < header.tracks.size() - 1; i++) {
			const auto& anchor = header.tracks[anchor_index];
			const auto& current = header.tracks[i];
			const auto& next = header.tracks[i + 1];

			if (next.frame > sequences[current_sequence].end_frame) {
				current_sequence += 1;
				// Write the end keyframe of the sequence
				header.tracks[write_index++] = current;
				// Write the start keyframe of the next sequence
				header.tracks[write_index++] = next;
				anchor_index = i + 1;
				// We increase loop counter i by 2
				i += 1;
				continue;
			}

			const auto total_span = static_cast<float>(next.frame - anchor.frame);
			const auto current_span = static_cast<float>(current.frame - anchor.frame);

			// Avoid division by zero
			const float t = (total_span > 0.0f) ? current_span / total_span : 0.0f;

			const auto interpolated = interpolate(anchor.value, anchor.outTan, anchor.inTan, next.value, t, static_cast<int>(header.interpolation_type));

			float error_sq;
			if constexpr (std::is_same_v<T, uint32_t>) {
				error_sq = (current.value - interpolated) * (current.value - interpolated);
			} else {
				error_sq = glm::length2(current.value - interpolated);
			}

			if (error_sq > max_error_sq) {
				header.tracks[write_index++] = current;
				anchor_index = i;
			}
		}

		// Always keep the last keyframe
		header.tracks[write_index++] = header.tracks.back();
		header.tracks.resize(write_index);
	}

	// ToDo! Make them sequence aware as now it will optimize all the frames of all sequences as if it was one sequence
	// ToDo! be aware of global sequences

	// ToDo! remove tracks outside of a sequence
	MDX::OptimizationStats MDX::optimize(float max_error) {
		OptimizationStats stats;

		if (sequences.empty()) {
			return stats;
		}

		for_each_track([&, max_error]<typename T>(TrackHeader<T>& header) {
			const auto track_size = header.tracks.size();
			reduce_track(header, sequences, max_error);

			if (header.interpolation_type == InterpolationType::none) {
				stats.constant_tracks += track_size;
				stats.constant_tracks_removed += track_size - header.tracks.size();
			} if (header.interpolation_type == InterpolationType::linear) {
				stats.linear_tracks += track_size;
				stats.linear_tracks_removed += track_size - header.tracks.size();
			} else if (header.interpolation_type == InterpolationType::hermite) {
				stats.hermite_tracks += track_size;
				stats.hermite_tracks_removed += track_size - header.tracks.size();
			} else if (header.interpolation_type == InterpolationType::bezier) {
				stats.bezier_tracks += track_size;
				stats.bezier_tracks_removed += track_size - header.tracks.size();
			}
		});

		return stats;
	}
}