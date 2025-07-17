module MDX;

import std;
import "glm/glm.hpp";

namespace mdx {
	/// Will not retain sequences and bones because there is no straightforward way to merge them
	void MDX::merge_with(const MDX& mdx, const glm::mat4& transform) {
		MDX new_mdx = mdx;
		for (auto& geoset : new_mdx.geosets) {
			geoset.material_id += materials.size();

			geoset.matrix_groups.clear();
			geoset.matrix_indices.clear();
			geoset.vertex_groups.clear();

			geoset.sequence_extents.resize(1);

			for (auto& vertex : geoset.vertices) {
				vertex = transform * glm::vec4(vertex, 1.f);
			}
			for (auto& normal : geoset.normals) {
				normal = glm::normalize(normal * glm::mat3(transform));
			}

			// Zero skin weights because we will have only 1 bone
			geoset.skin = std::vector<std::uint8_t>(geoset.vertices.size() * 8, 0);
			// Set the contribution of the first bone to 255
			for (size_t i = 0; i < geoset.skin.size(); i += 8) {
				geoset.skin[i + 4] = 255;
			}
		}

		for (auto& material : new_mdx.materials) {
			for (auto& layer : material.layers) {
				layer.texture_animation_id = 0;
				layer.texture_animation_id += texture_animations.size();
				for (auto& texture : layer.texturess) {
					texture.id += textures.size();
				}
			}
		}

		// for (auto & emitter : new_mdx.emitters2) {
		// 	emitter.texture_id += textures.size();
		// }
		//
		// for (auto & ribbon : new_mdx.ribbons) {
		// 	ribbon.material_id += materials.size();
		// }

		geosets.append_range(new_mdx.geosets);
		// sequences.append_range(new_mdx.sequences);
		// global_sequences.append_range(new_mdx.global_sequences);
		animations.append_range(new_mdx.animations);
		// bones.append_range(new_mdx.bones);
		materials.append_range(new_mdx.materials);
		textures.append_range(new_mdx.textures);
		// lights.append_range(new_mdx.lights);
		// help_bones.append_range(new_mdx.help_bones);
		// attachments.append_range(new_mdx.attachments);
		// pivots.append_range(new_mdx.pivots);
		// emitters1.append_range(new_mdx.emitters1);
		// emitters2.append_range(new_mdx.emitters2);
		// ribbons.append_range(new_mdx.ribbons);
		// event_objects.append_range(new_mdx.event_objects);
		// collision_shapes.append_range(new_mdx.collision_shapes);
		// corn_emitters.append_range(new_mdx.corn_emitters);
		// facefxes.append_range(new_mdx.facefxes);
		// cameras.append_range(new_mdx.cameras);
		// bind_poses.append_range(new_mdx.bind_poses);
		// texture_animations.append_range(new_mdx.texture_animations);

		// Just to be sure
		validate();
	}

	/// Technically SD supports infinite bones per vertex, but we limit it to 4 like HD does.
	/// This could cause graphical inconsistencies with the game, but after more than 4 bones the contribution per bone is low enough that we don't care
	std::vector<glm::u8vec4> MDX::matrix_groups_as_skin_weights(const Geoset& geoset) {
		std::vector<glm::u8vec4> groups;
		groups.reserve(geoset.matrix_groups.size());
		std::vector<glm::u8vec4> weights;
		weights.reserve(geoset.matrix_groups.size());

		size_t bone_offset = 0;
		for (const auto& group_size : geoset.matrix_groups) {
			const int bone_count = std::min(group_size, 4u);
			glm::uvec4 indices(0);
			glm::uvec4 weightss(0);

			const int weight = 255 / bone_count;
			for (size_t j = 0; j < bone_count; j++) {
				indices[j] = geoset.matrix_indices[bone_offset + j];
				weightss[j] = weight;
			}
			weightss[0] += 255 % bone_count;

			groups.push_back(indices);
			weights.push_back(weightss);
			bone_offset += group_size;
		}

		std::vector<glm::u8vec4> skin_weights;
		skin_weights.reserve(groups.size() * 2);
		for (const auto& vertex_group : geoset.vertex_groups) {
			skin_weights.push_back(groups[vertex_group]);
			skin_weights.push_back(weights[vertex_group]);
		}

		return skin_weights;
	}

	MDX& MDX::calculate_extents() {
		for (auto& geoset : geosets) {
			geoset.extent.minimum = glm::vec3(std::numeric_limits<float>::max());
			geoset.extent.maximum = glm::vec3(std::numeric_limits<float>::lowest());
			for (const auto& i : geoset.vertices) {
				geoset.extent.minimum = glm::min(geoset.extent.minimum, i);
				geoset.extent.maximum = glm::max(geoset.extent.maximum, i);
			}

			geoset.extent.bounds_radius =
				std::max(glm::distance(glm::vec3(0.0), geoset.extent.minimum), glm::distance(glm::vec3(0.0), geoset.extent.maximum));

			for (auto& extent : geoset.sequence_extents) {
				// Wrong because we should capture the min/max of the entire animation but that's kind of a pain to implement
				extent = geoset.extent;
			}

			extent.minimum = glm::min(extent.minimum, geoset.extent.minimum);
			extent.maximum = glm::max(extent.maximum, geoset.extent.maximum);
			extent.bounds_radius = std::max(extent.bounds_radius, geoset.extent.bounds_radius);
		}

		for (auto& sequence : sequences) {
			// Wrong because we should capture the min/max of the entire animation but that's kind of a pain to implement
			sequence.extent = extent;
		}

		return *this;
	}
} // namespace mdx