module;

// #include <QDesktopServices>
// #include <QStandardPaths>
// #include <qurl.h>
// #include <filesystem>

module MDX;

import std;
// import types;
import "glm/glm.hpp";

namespace mdx {
	/// Will delete sequences and bones because there is no straightforward way to merge them
	void MDX::merge_with(const MDX& mdx, const glm::mat4& transform) {
		sequences.clear();
		global_sequences.clear();
		animations.clear();
		bones.clear();
		lights.clear();
		help_bones.clear();
		attachments.clear();
		pivots.clear();
		emitters1.clear();
		emitters2.clear();
		ribbons.clear();
		event_objects.clear();
		collision_shapes.clear();
		corn_emitters.clear();
		facefxes.clear();
		cameras.clear();
		bind_poses.clear();
		texture_animations.clear();

		// animations.push_back(GeosetAnimation {
		// 	.alpha = 1.f,
		// 	.flags = 0,
		// 	.color = glm::vec3(1.f,1.f,1.f),
		// 	.geoset_id = 0,
		// });

		// validate();

		// auto mdl = to_mdl();

		// auto path = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + QString::fromStdString(name) + ".mdl";
		//
		// std::ofstream file(path.toStdString());
		// file.write(mdl.data(), mdl.size());
		// file.close();
		//
		// QDesktopServices::openUrl(QUrl(path, QUrl::TolerantMode));
		//
		// return;

		for (auto& geoset : geosets) {
			// Zero skin weights because we will have only 1 bone
			geoset.skin = std::vector<std::uint8_t>(geoset.vertices.size(), 0);
			// Set the contribution of the first bone to 255
			for (size_t i = 0; i < geoset.skin.size(); i += 8) {
				geoset.skin[i + 4] = 255;
			}
		}

		MDX new_mdx = mdx;
		for (auto& geoset : new_mdx.geosets) {
			// geoset.material_id += materials.size();
			geoset.matrix_groups.clear();
			geoset.matrix_indices.clear();

			for (auto & vertex : geoset.vertices) {
				vertex = glm::vec4(vertex, 1.f) * transform;
			}
			for (auto & normal : geoset.normals) {
				normal = glm::normalize(normal * glm::mat3(transform));
			}

			// if (!geoset.matrix_groups.empty()) {
			// 	geoset.skin = MDX::matrix_groups_as_skin_weights(geoset);
			// }

			// Zero skin weights because we will have only 1 bone
			geoset.skin = std::vector<std::uint8_t>(geoset.vertices.size(), 0);
			// Set the contribution of the first bone to 255
			for (size_t i = 0; i < geoset.skin.size(); i += 8) {
				geoset.skin[i + 4] = 255;
			}
		}

		// for (auto& animation : new_mdx.animations) {
		// 	animation.geoset_id += geosets.size();
		// }

		// for (auto& bone : new_mdx.bones) {
		// 	bone.geoset_id += geosets.size();
		// 	bone.geoset_animation_id += animations.size();
		// }

		for (auto & material : new_mdx.materials) {
			for (auto & layer : material.layers) {
				layer.texture_animation_id = 0;
				layer.texture_animation_id += texture_animations.size();
				for (auto & texture : layer.texturess) {
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

		// This doesn't merge the sequences but only appends them
		// You cannot play an animation for any of the containing MDXs at the same time
		// const auto max_frame = sequences.back().end_frame;
		// for (auto & sequence : new_mdx.sequences) {
		// 	sequence.start_frame += max_frame;
		// 	sequence.end_frame += max_frame;
		// }
		//
		// new_mdx.for_each_track([&]<typename T>(TrackHeader<T>& header) {
		// 	header.global_sequence_ID += global_sequences.size();
		// 	header.id += unique_tracks;
		// 	for (auto& track : header.tracks) {
		// 		track.frame += max_frame;
		// 	}
		// });
		// unique_tracks += new_mdx.unique_tracks;
		//
		// size_t node_count = new_mdx.bones.size() +
		// 					new_mdx.lights.size() +
		// 					new_mdx.help_bones.size() +
		// 					new_mdx.attachments.size() +
		// 					new_mdx.emitters1.size() +
		// 					new_mdx.emitters2.size() +
		// 					new_mdx.ribbons.size() +
		// 					new_mdx.event_objects.size() +
		// 					new_mdx.collision_shapes.size() +
		// 					new_mdx.corn_emitters.size();
		//
		// new_mdx.for_each_node([node_count](auto& node) {
		// 	node.id += node_count;
		// 	node.parent_id += node_count;
		// });

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
	// std::vector<unsigned char> MDX::matrix_groups_as_skin_weights(const Geoset& geoset) {
		// std::vector<glm::u8vec4> groups;
		// std::vector<glm::u8vec4> weights;
		//
		// size_t bone_offset = 0;
		// for (const auto& group_size : geoset.matrix_groups) {
		// 	const int bone_count = std::min(group_size, 4u);
		// 	glm::uvec4 indices(0);
		// 	glm::uvec4 weightss(0);
		//
		// 	const int weight = 255 / bone_count;
		// 	for (size_t j = 0; j < bone_count; j++) {
		// 		indices[j] = geoset.matrix_indices[bone_offset + j];
		// 		weightss[j] = weight;
		// 	}
		//
		// 	const int remainder = 255 - weight * bone_count;
		// 	weightss[0] += remainder;
		//
		// 	groups.push_back(indices);
		// 	weights.push_back(weightss);
		// 	bone_offset += group_size;
		// }
		//
		// std::vector<glm::u8vec4> skin_weights;
		// skin_weights.reserve(groups.size() * 2);
		// for (const auto& vertex_group : geoset.vertex_groups) {
		// 	skin_weights.push_back(groups[vertex_group]);
		// 	skin_weights.push_back(weights[vertex_group]);
		// }
		//
		// // return skin_weights;
		// std::vector<unsigned char> flattened;
		// flattened.reserve(skin_weights.size() * 4);
		// for (const auto& vec : skin_weights) {
		// 	flattened.push_back(vec[0]);
		// 	flattened.push_back(vec[1]);
		// 	flattened.push_back(vec[2]);
		// 	flattened.push_back(vec[3]);
		// }
		// return flattened;
		// return {};
	// }
}