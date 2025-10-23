module;

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

module MDX;

import std;
import types;
import MathOperations;
import <glm/glm.hpp>;
import <glm/gtc/quaternion.hpp>;

namespace mdx {
	void remove_unused_materials(MDX& mdx) {
		if (mdx.geosets.empty()) {
			return;
		}

		std::set<u32> used_materials;
		for (const auto& geoset : mdx.geosets) {
			used_materials.emplace(geoset.material_id);
		}

		std::vector<u32> mapping(mdx.materials.size(), 0);
		size_t current_slot = 0;
		for (const auto& i : used_materials) {
			mapping[i] = current_slot++;
		}

		for (auto& geoset : mdx.geosets) {
			geoset.material_id = mapping[geoset.material_id];
		}

		size_t write = 0;
		for (size_t read = 0; read < mdx.materials.size(); ++read) {
			if (used_materials.contains(read)) {
				mdx.materials[write++] = mdx.materials[read];
			}
		}
		mdx.materials.resize(write);
	}

	/// Removes textures that aren't referenced by any MDX materials
	/// ToDo also check if textures are used by other MDX chunks
	void remove_unused_textures(MDX& mdx) {
		if (mdx.textures.empty()) {
			return;
		}

		std::set<u32> used_textures;
		for (const auto& material : mdx.materials) {
			for (const auto& layer : material.layers) {
				for (const auto& texture : layer.textures) {
					used_textures.emplace(texture.id);
				}
			}
		}

		std::vector<u32> mapping(mdx.textures.size(), 0);
		size_t current_slot = 0;
		for (const auto& i : used_textures) {
			mapping[i] = current_slot++;
		}

		for (auto& material : mdx.materials) {
			for (auto& layer : material.layers) {
				for (auto& texture : layer.textures) {
					texture.id = mapping[texture.id];
				}
			}
		}

		size_t write = 0;
		for (size_t read = 0; read < mdx.textures.size(); ++read) {
			if (used_textures.contains(read)) {
				mdx.textures[write++] = mdx.textures[read];
			}
		}
		mdx.textures.resize(write);
	}

	/// Deduplicate materials and geosets before this for best results
	MDX& MDX::deduplicate_textures() {
		std::unordered_map<Texture, size_t> texture_map;
		std::vector<u32> mapping;

		for (const auto& texture : textures) {
			const auto& [elem, inserted] = texture_map.emplace(texture, mapping.size());
			mapping.push_back(elem->second);
		}

		for (auto& material : materials) {
			for (auto& layer : material.layers) {
				for (auto& texture : layer.textures) {
					texture.id = mapping[texture.id];
				}
			}
		}

		textures.clear();
		for (const auto& [key, value] : texture_map) {
			textures.push_back(key);
		}
		return *this;
	}

	/// Call after deduplicate_textures for best results
	/// as the texture IDs will be deduplicated so fewer differences
	/// Material IDs will be sequential, from zero, without gaps after this
	MDX& MDX::deduplicate_materials() {
		std::unordered_map<Material, size_t> material_map;
		std::vector<u32> mapping;

		for (const auto& material : materials) {
			const auto& [elem, inserted] = material_map.emplace(material, mapping.size());
			mapping.push_back(elem->second);
		}

		for (auto& geoset : geosets) {
			geoset.material_id = mapping[geoset.material_id];
		}

		materials.clear();
		for (const auto& [key, value] : material_map) {
			materials.push_back(key);
		}
		return *this;
	}

	/// Call after deduplicate_materials() for best results.
	/// Does not guarantee correct results if the model has any kind of animation
	/// as it merges geosets purely based on matching material IDs.
	/// Requires material IDs to be sequential, from zero, without gaps.
	/// Requires recalculating extents after.
	MDX& MDX::deduplicate_geosets() {
		std::ranges::sort(geosets, [](const auto& a, const auto& b) {
			return a.material_id < b.material_id;
		});

		size_t i = 0;
		while (i < geosets.size()) {
			const size_t index = geosets[i].material_id;

			if (i == index) {
				i += 1;
				continue;
			}

			geosets[index].vertices.append_range(geosets[i].vertices);
			geosets[index].normals.append_range(geosets[i].normals);
			geosets[index].face_type_groups.append_range(geosets[i].face_type_groups);
			// Just use triangles
			// geosets[current_material].face_groups.append_range(geosets[i].face_groups);
			geosets[index].faces.append_range(geosets[i].faces);
			// The following three lines are likely wrong but w/e, you should use skin anyway
			geosets[index].vertex_groups.append_range(geosets[i].vertex_groups);
			geosets[index].matrix_groups.append_range(geosets[i].matrix_groups);
			geosets[index].matrix_indices.append_range(geosets[i].matrix_indices);

			// We told the user to recalculate extents after
			// geosets[index].extents.append_range(geosets[i].extents);

			geosets[index].tangents.append_range(geosets[i].tangents);
			geosets[index].skin.append_range(geosets[i].skin);

			// By the lord, please don't use multiple uv sets
			assert(geosets[index].uv_sets.size() == geosets[i].uv_sets.size());

			for (size_t j = 0; j < geosets[i].uv_sets.size(); j++) {
				geosets[index].uv_sets[j].append_range(geosets[i].uv_sets[j]);
			}

			geosets.erase(geosets.begin() + i);
		}

		return *this;
	}

	/// Removes tracks that are not inside a sequence start<->end frame
	template <typename T>
	void remove_tracks_outside_sequences(TrackHeader<T>& header, std::vector<Sequence>& sequences) {
		if (sequences.empty()) {
			return;
		}

		size_t write_index = 0;
		size_t current_sequence = 0;

		for (size_t i = 0; i < header.tracks.size(); i++) {
			const auto& current = header.tracks[i];

			if (current.frame > sequences[current_sequence].end_frame) {
				current_sequence += 1;
				if (current_sequence >= sequences.size()) {
					return;
				}
			}

			if (current.frame < sequences[current_sequence].start_frame) {
				continue;
			}

			header.tracks[write_index++] = current;
		}
	}

	/// Reduces the number of keyframes in a track by decimating the curve
	/// Uses the Douglas–Peucker algorithm
	/// Assumes all tracks are contained in a sequence
	template <typename T>
	void reduce_track(TrackHeader<T>& header, std::vector<Sequence>& sequences, float max_error_sq) {
		if (sequences.empty()) {
			return;
		}

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
				error_sq = current.value - interpolated;
			} else {
				error_sq = glm::length(current.value - interpolated);
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

	// ToDo! be aware of global sequences

	MDX::OptimizationStats MDX::optimize(float max_error) {
		OptimizationStats stats;

		const size_t material_count = materials.size();
		remove_unused_materials(*this);
		stats.materials_removed += material_count - materials.size();

		const size_t texture_count = textures.size();
		remove_unused_textures(*this);
		stats.textures_removed += texture_count - textures.size();

		if (sequences.empty()) {
			return stats;
		}

		for_each_track([&, max_error]<typename T>(TrackHeader<T>& header) {
			const auto track_size = header.tracks.size();

			remove_tracks_outside_sequences(header, sequences);
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