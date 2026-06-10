module;

module MDX;

import std;

namespace mdx {
	/// Returns false is the model has errors so severe that it cannot be displayed at all
	bool MDX::is_valid() {
		bool is_valid = true;

		const size_t node_count = bones.size() + lights.size() + help_bones.size() + attachments.size() + emitters1.size()
			+ emitters2.size() + ribbons.size() + event_objects.size() + collision_shapes.size() + corn_emitters.size();

		// Check for missing/incorrect/duplicate Node IDs
		{
			int64_t sum = 0;
			int64_t sum_squared = 0;
			for_each_node([&](const Node& node) {
				if (node.id < 0 || node.id >= node_count) {
					is_valid = false;
				}

				if (node.parent_id < -1) {
					is_valid = false;
				}

				if (node.parent_id >= 0 && node.parent_id >= node_count) {
					is_valid = false;
				}

				if (node.parent_id == node.id) {
					is_valid = false;
				}

				sum += node.id;
				sum_squared += static_cast<int64_t>(node.id) * node.id;
			});
			const int64_t correct_sum = node_count * (node_count - 1) / 2;
			const int64_t correct_sum_squared = (node_count - 1) * node_count * (2 * node_count - 1) / 6;
			is_valid = is_valid && sum == correct_sum && sum_squared == correct_sum_squared;
		}

		for (const auto& material : materials) {
			for (const auto& layer: material.layers) {
				for (const auto& texture: layer.textures) {
					if (texture.id >= textures.size()) {
						is_valid = false;
					}
				}
			}
		}

		for (const auto& emitter : emitters2) {
			// HiveWE specific?
			if (emitter.life_span < 0.0) {
				is_valid = false;
			}
			// HiveWE specific?
			if (emitter.time_middle < 0.0 || emitter.time_middle > 1.f) {
				is_valid = false;
			}
			// HiveWE specific?
			if (emitter.rows == 0 || emitter.columns == 0) {
				is_valid = false;
			}
			if (emitter.texture_id >= textures.size()) {
				is_valid = false;
			}
			// HiveWE specific?
			if (emitter.squirt > 1) {
				is_valid = false;
			}
			// HiveWE specific?
			if (emitter.tail_length < 0.f) {
				is_valid = false;
			}
		}

		return is_valid;
	}

	/// Returns a list of model errors and warnings at different severity levels.
	std::vector<ValidationMessage> MDX::validate() {
		std::vector<ValidationMessage> messages;

		const auto error = [&](std::string message) {
			messages.push_back({ ValidationSeverity::error, std::move(message) });
		};
		const auto severe = [&](std::string message) {
			messages.push_back({ ValidationSeverity::severe, std::move(message) });
		};
		const auto warning = [&](std::string message) {
			messages.push_back({ ValidationSeverity::warning, std::move(message) });
		};
		const auto unused = [&](std::string message) {
			messages.push_back({ ValidationSeverity::unused, std::move(message) });
		};

		const size_t node_count = bones.size() + lights.size() + help_bones.size() + attachments.size() + emitters1.size()
			+ emitters2.size() + ribbons.size() + event_objects.size() + collision_shapes.size() + corn_emitters.size();

		// Bone node IDs, used to validate skinning references and detect unused bones.
		std::unordered_set<int> bone_ids;
		for (const auto& bone : bones) {
			bone_ids.insert(bone.node.id);
		}
		std::unordered_set<int> used_bones; // Bone node IDs that have at least one vertex attached.

		// Nodes (fatal)
		std::unordered_set<int> ids;

		for_each_node([&](const Node& node) {
			if (node.id < 0) {
				error(std::format("Node \"{}\" has invalid ID {}", node.name, node.id));
			}
			if (node.id >= node_count) {
				error(std::format("Node \"{}\" has ID {} which is higher than node count {}", node.name, node.id, node_count));
			}
			const auto [id, inserted] = ids.insert(node.id);
			if (!inserted) {
				error(std::format("Node {} has duplicated ID {}", node.name, node.id));
			}

			if (node.parent_id < -1) {
				error(std::format("Node {} references invalid parent ID {}", node.name, node.parent_id));
			}

			if (node.parent_id >= 0 && node.parent_id >= node_count) {
				error(std::format("Node {} references invalid parent ID {}", node.name, node.parent_id));
			}

			if (node.parent_id == node.id) {
				error(std::format("Node {} references itself as parent {}", node.name, node.parent_id));
			}
		});

		// Materials (fatal texture refs)
		for (const auto& [id, material] : std::ranges::enumerate_view(materials)) {
			if (material.layers.empty()) {
				error(std::format("Material {} has no layers", id));
			}

			for (const auto& [layer_id, layer] : std::ranges::enumerate_view(material.layers)) {
				if (layer.texture_animation_id != 0xFFFFFFFF && layer.texture_animation_id >= texture_animations.size()) {
					warning(std::format("Material {} layer {} references invalid texture animation id {}", id, layer_id, layer.texture_animation_id));
				}

				for (const auto& texture : layer.textures) {
					if (texture.id >= textures.size()) {
						error(std::format("Material {} has invalid texture id {}", id, texture.id));
					}
					if (texture.slot > 5) {
						warning(std::format("Material {} layer {} texture uses invalid PBR slot {}", id, layer_id, texture.slot));
					}
				}
			}
		}

		// Textures
		static const std::unordered_set<uint32_t> valid_replaceable_ids = { 1, 2, 11, 21, 31, 32, 33, 34, 35, 36, 37 };
		for (const auto& [id, texture] : std::ranges::enumerate_view(textures)) {
			if (texture.replaceable_id == 0 && texture.file_name.empty()) {
				warning(std::format("Texture {} has no replaceable id and no file path", id));
			}
			if (texture.replaceable_id != 0 && !texture.file_name.empty()) {
				warning(std::format("Texture {} has both a replaceable id ({}) and a file path", id, texture.replaceable_id));
			}
			if (texture.replaceable_id != 0 && !valid_replaceable_ids.contains(texture.replaceable_id)) {
				error(std::format("Texture {} has unknown replaceable id {}", id, texture.replaceable_id));
			}
			if (!texture.file_name.empty()) {
				auto ext = texture.file_name.extension().string();
				std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return std::tolower(c); });
				if (ext != ".blp" && ext != ".tga" && ext != ".tif" && ext != ".dds") {
					error(std::format("Texture {} has a corrupted path \"{}\"", id, texture.file_name.string()));
				}
			}
		}

		// Geosets
		for (const auto& [id, geoset] : std::ranges::enumerate_view(geosets)) {
			if (geoset.material_id >= materials.size()) {
				warning(std::format("Geoset {} references invalid material id {}", id, geoset.material_id));
			}

			if (geoset.vertices.empty()) {
				warning(std::format("Geoset {} has no vertices", id));
			}
			if (geoset.faces.empty()) {
				warning(std::format("Geoset {} has no faces", id));
			}

			if (geoset.normals.size() != geoset.vertices.size()) {
				warning(std::format("Geoset {} has {} normals but {} vertices", id, geoset.normals.size(), geoset.vertices.size()));
			}

			for (const auto& [set_id, uv_set] : std::ranges::enumerate_view(geoset.uv_sets)) {
				if (uv_set.size() != geoset.vertices.size()) {
					warning(std::format("Geoset {} UV set {} has {} coordinates but {} vertices", id, set_id, uv_set.size(), geoset.vertices.size()));
				}
			}

			if (!geoset.vertex_groups.empty() && geoset.vertex_groups.size() != geoset.vertices.size()) {
				warning(std::format("Geoset {} has {} vertex groups but {} vertices", id, geoset.vertex_groups.size(), geoset.vertices.size()));
			}

			if (!geoset.skin.empty() && geoset.skin.size() / 8 != geoset.vertices.size()) {
				warning(std::format("Geoset {} has {} skin weights but {} vertices", id, geoset.skin.size() / 8, geoset.vertices.size()));
			}

			for (const auto& face : geoset.faces) {
				if (face >= geoset.vertices.size()) {
					warning(std::format("Geoset {} has a face index {} that exceeds vertex count {}", id, face, geoset.vertices.size()));
					break;
				}
			}

			for (const auto& group : geoset.vertex_groups) {
				if (group >= geoset.matrix_groups.size()) {
					warning(std::format("Geoset {} has a vertex group {} that exceeds matrix group count {}", id, group, geoset.matrix_groups.size()));
					break;
				}
			}

			if (geoset.faces.size() % 3 != 0) {
				warning(std::format("Geoset {} has {} face indices, which is not a multiple of 3", id, geoset.faces.size()));
			}

			if (version == 800 && geoset.sequence_extents.size() != sequences.size()) {
				warning(std::format("Geoset {} has {} sequence extents but the model has {} sequences", id, geoset.sequence_extents.size(), sequences.size()));
			}

			const int64_t referencing_animations = std::ranges::count_if(animations, [&](const GeosetAnimation& a) {
				return a.geoset_id == static_cast<uint32_t>(id);
			});
			if (referencing_animations > 1) {
				warning(std::format("Geoset {} is referenced by {} geoset animations", id, referencing_animations));
			}

			const bool is_hd = geoset.material_id < materials.size() && !materials[geoset.material_id].layers.empty()
				&& materials[geoset.material_id].layers.front().shader == ShaderType::HD;

			// SD geosets above a certain size render bugged in Warcraft 3.
			constexpr size_t sd_max_vertices = 7433;
			if (!is_hd && geoset.vertices.size() >= sd_max_vertices) {
				severe(std::format("Geoset {} has {} vertices, which exceeds the Warcraft 3 limit for SD geosets", id, geoset.vertices.size()));
			}

			// Skinning references: HD geosets use skin weights, SD geosets use matrix groups.
			if (version > 800 && !geoset.skin.empty()) {
				for (size_t v = 0, count = geoset.skin.size() / 8; v < count; v++) {
					const size_t offset = v * 8;
					int weight_sum = 0;
					for (size_t j = 0; j < 4; j++) {
						const uint8_t bone = geoset.skin[offset + j];
						const uint8_t weight = geoset.skin[offset + 4 + j];
						weight_sum += weight;
						if (weight == 0) {
							continue;
						}
						if (bone >= node_count) {
							error(std::format("Geoset {} vertex {} is attached to object {} which does not exist", id, v, bone));
						} else if (!bone_ids.contains(bone)) {
							severe(std::format("Geoset {} vertex {} is attached to object {} which is not a bone", id, v, bone));
						} else {
							used_bones.insert(bone);
						}
					}
					if (weight_sum == 0) {
						severe(std::format("Geoset {} vertex {} is not attached to anything", id, v));
					} else if (weight_sum != 255) {
						severe(std::format("Geoset {} vertex {} has weights that are not normalized (sum {})", id, v, weight_sum));
					}
				}
			} else {
				for (const auto& index : geoset.matrix_indices) {
					if (index >= node_count) {
						error(std::format("Geoset {} matrix index {} references a node that does not exist", id, index));
					} else if (!bone_ids.contains(static_cast<int>(index))) {
						severe(std::format("Geoset {} matrix index {} references a node that is not a bone", id, index));
					} else {
						used_bones.insert(static_cast<int>(index));
					}
				}
			}
		}

		// Geoset animations
		for (const auto& [id, animation] : std::ranges::enumerate_view(animations)) {
			if (animation.geoset_id >= geosets.size()) {
				warning(std::format("Geoset animation {} references invalid geoset id {}", id, animation.geoset_id));
			}
		}

		// Bones
		const bool has_skinnable_geometry = std::ranges::any_of(geosets, [](const Geoset& g) { return !g.vertices.empty(); });
		for (const auto& [id, bone] : std::ranges::enumerate_view(bones)) {
			if (bone.geoset_id != -1 && bone.geoset_id >= static_cast<int32_t>(geosets.size())) {
				warning(std::format("Bone {} references invalid geoset id {}", id, bone.geoset_id));
			}
			if (bone.geoset_animation_id != -1 && bone.geoset_animation_id >= static_cast<int32_t>(animations.size())) {
				warning(std::format("Bone {} references invalid geoset animation id {}", id, bone.geoset_animation_id));
			}
			if (has_skinnable_geometry && !used_bones.contains(bone.node.id)) {
				warning(std::format("Bone {} \"{}\" has no vertices attached", id, bone.node.name));
			}
		}

		// Particle/ribbon emitters
		const auto has_model_extension = [](const std::string& path) {
			std::string lower = path;
			std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return std::tolower(c); });
			return lower.ends_with(".mdl") || lower.ends_with(".mdx");
		};

		for (const auto& emitter : emitters1) {
			if (!has_model_extension(emitter.path)) {
				error(std::format("Particle emitter \"{}\" has an invalid model path \"{}\"", emitter.node.name, emitter.path));
			}
		}

		for (const auto& emitter : emitters2) {
			if (emitter.texture_id >= textures.size()) {
				error(std::format("Particle emitter 2 \"{}\" references invalid texture id {}", emitter.node.name, emitter.texture_id));
			}
			if (emitter.replaceable_id != 0 && !valid_replaceable_ids.contains(emitter.replaceable_id)) {
				error(std::format("Particle emitter 2 \"{}\" has invalid replaceable id {}", emitter.node.name, emitter.replaceable_id));
			}
			if (emitter.time_middle < 0.f || emitter.time_middle > 1.f) {
				severe(std::format("Particle emitter 2 \"{}\" has a time middle {} outside [0, 1]", emitter.node.name, emitter.time_middle));
			}
			if (emitter.life_span < 0.f) {
				severe(std::format("Particle emitter 2 \"{}\" has a lifespan {} that is negative", emitter.node.name, emitter.life_span));
			}
			if (emitter.rows == 0 || emitter.columns == 0) {
				warning(std::format("Particle emitter 2 \"{}\" has zero rows or columns", emitter.node.name));
			}
			if (emitter.squirt > 1) {
				warning(std::format("Particle emitter 2 \"{}\" has squirt that is {}, should be 0 or 1", emitter.node.name, emitter.squirt));
			}
			if (emitter.tail_length < 0.f) {
				warning(std::format("Particle emitter 2 \"{}\" has tail length that is {}, should be larger than 0", emitter.node.name, emitter.tail_length));
			}
		}

		for (const auto& ribbon : ribbons) {
			if (ribbon.material_id >= materials.size()) {
				warning(std::format("Ribbon emitter \"{}\" references invalid material id {}", ribbon.node.name, ribbon.material_id));
			}
		}

		// Lights
		for (const auto& [id, light] : std::ranges::enumerate_view(lights)) {
			if (light.attenuation_start > light.attenuation_end) {
				warning(std::format("Light \"{}\" has attenuation start {} greater than end {}", light.node.name, light.attenuation_start, light.attenuation_end));
			}
		}

		// Attachments
		for (const auto& attachment : attachments) {
			if (!attachment.path.empty() && !has_model_extension(attachment.path)) {
				error(std::format("Attachment \"{}\" has an invalid path \"{}\"", attachment.node.name, attachment.path));
			}
		}

		// Cameras
		for (const auto& camera : cameras) {
			if (camera.near_clip < 0.f || camera.far_clip <= camera.near_clip) {
				warning(std::format("Camera \"{}\" has invalid clip planes (near {}, far {})", camera.name, camera.near_clip, camera.far_clip));
			}
		}

		// Event objects
		for (const auto& event : event_objects) {
			if (event.global_sequence_id != -1 && static_cast<size_t>(event.global_sequence_id) >= global_sequences.size()) {
				warning(std::format("Event object \"{}\" references invalid global sequence id {}", event.node.name, event.global_sequence_id));
			}
			if (event.times.empty()) {
				error(std::format("Event object \"{}\" has no event tracks", event.node.name));
			}
			bool negative_reported = false;
			bool order_reported = false;
			for (size_t i = 0; i < event.times.size(); i++) {
				if (!negative_reported && event.times[i] < 0) {
					warning(std::format("Event object \"{}\" has a negative event time {}", event.node.name, event.times[i]));
					negative_reported = true;
				}
				if (!order_reported && i > 0 && event.times[i] <= event.times[i - 1]) {
					severe(std::format("Event object \"{}\" has event times that are not strictly increasing", event.node.name));
					order_reported = true;
				}
			}
		}

		// Animation tracks
		for_each_track([&](const auto& header) {
			if (header.global_sequence_ID != -1
				&& (header.global_sequence_ID < 0 || static_cast<size_t>(header.global_sequence_ID) >= global_sequences.size())) {
				warning(std::format("A track references invalid global sequence id {}", header.global_sequence_ID));
			}

			bool order_reported = false;
			bool duplicate_reported = false;
			for (size_t i = 1; i < header.tracks.size(); i++) {
				if (!order_reported && header.tracks[i].frame < header.tracks[i - 1].frame) {
					severe(std::format("A track has keyframes that are not in ascending order (frame {} after {})", header.tracks[i].frame, header.tracks[i - 1].frame));
					order_reported = true;
				}
				if (!duplicate_reported && header.tracks[i].frame == header.tracks[i - 1].frame) {
					warning(std::format("A track has a duplicate keyframe at frame {}", header.tracks[i].frame));
					duplicate_reported = true;
				}
			}
		});

		// Pivots & sequences
		if (pivots.size() != node_count) {
			warning(std::format("Model has {} pivot points but {} nodes", pivots.size(), node_count));
		}

		for (const auto& [id, sequence] : std::ranges::enumerate_view(sequences)) {
			if (sequence.end_frame <= sequence.start_frame) {
				warning(std::format("Sequence \"{}\" has a non-positive interval [{}, {}]", sequence.name, sequence.start_frame, sequence.end_frame));
			}

			// Pre-Reforged the game is picky about sequence interval ordering.
			if (version == 800) {
				for (int64_t j = 0; j < id; j++) {
					if (sequence.start_frame == sequences[j].start_frame) {
						severe(std::format("Sequence \"{}\" starts at the same frame as sequence {} \"{}\"", sequence.name, j, sequences[j].name));
					} else if (sequence.start_frame < sequences[j].end_frame) {
						severe(std::format("Sequence \"{}\" starts before sequence {} \"{}\" ends", sequence.name, j, sequences[j].name));
					}
				}
			}
		}

		for (const auto& [id, duration] : std::ranges::enumerate_view(global_sequences)) {
			if (duration == 0) {
				warning(std::format("Global sequence {} has zero duration", id));
			}
		}

		// Extents
		const auto check_extent = [&](const std::string& label, const Extent& e) {
			if (e.maximum.x < e.minimum.x || e.maximum.y < e.minimum.y || e.maximum.z < e.minimum.z) {
				warning(std::format("{} has negative extents", label));
			}
		};
		check_extent("Model", extent);
		for (const auto& [id, sequence] : std::ranges::enumerate_view(sequences)) {
			check_extent(std::format("Sequence \"{}\"", sequence.name), sequence.extent);
		}
		for (const auto& [id, geoset] : std::ranges::enumerate_view(geosets)) {
			check_extent(std::format("Geoset {}", id), geoset.extent);
			for (const auto& [eid, e] : std::ranges::enumerate_view(geoset.sequence_extents)) {
				check_extent(std::format("Geoset {} sequence extent {}", id, eid), e);
			}
		}

		// Node hierarchy cycles
		std::unordered_map<int, int> parent_of;
		for_each_node([&](const Node& node) {
			parent_of[node.id] = node.parent_id;
		});
		bool cycle_reported = false;
		for_each_node([&](const Node& node) {
			if (cycle_reported || node.parent_id == -1 || node.parent_id == node.id) {
				return;
			}
			std::unordered_set<int> seen;
			int current = node.id;
			for (size_t steps = 0; steps <= node_count && current != -1; steps++) {
				if (!seen.insert(current).second) {
					severe(std::format("Node \"{}\" is part of a parent cycle", node.name));
					cycle_reported = true;
					return;
				}
				const auto it = parent_of.find(current);
				if (it == parent_of.end()) {
					break;
				}
				current = it->second;
			}
		});

		// Bind pose (HD)
		if (version >= 900 && !bind_poses.empty()) {
			// There is conventionally one extra matrix beyond the node count.
			const size_t matrices = bind_poses.size() / 12;
			if (matrices != node_count + 1) {
				warning(std::format("Model has {} bind pose matrices but expected {} (node count + 1)", matrices, node_count + 1));
			}
		}

		// Unused objects
		std::unordered_set<uint32_t> used_textures;
		std::unordered_set<uint32_t> used_materials;
		std::unordered_set<uint32_t> used_texture_animations;
		for (const auto& material : materials) {
			for (const auto& layer : material.layers) {
				if (layer.texture_animation_id != 0xFFFFFFFF) {
					used_texture_animations.insert(layer.texture_animation_id);
				}
				for (const auto& texture : layer.textures) {
					used_textures.insert(texture.id);
					for (const auto& track : texture.KMTF.tracks) {
						used_textures.insert(track.value);
					}
				}
			}
		}
		for (const auto& emitter : emitters2) {
			used_textures.insert(emitter.texture_id);
		}
		for (const auto& geoset : geosets) {
			used_materials.insert(geoset.material_id);
		}
		for (const auto& ribbon : ribbons) {
			used_materials.insert(ribbon.material_id);
		}
		for (size_t i = 0; i < textures.size(); i++) {
			if (!used_textures.contains(static_cast<uint32_t>(i))) {
				unused(std::format("Texture {} is never referenced", i));
			}
		}
		for (size_t i = 0; i < materials.size(); i++) {
			if (!used_materials.contains(static_cast<uint32_t>(i))) {
				unused(std::format("Material {} is never referenced", i));
			}
		}
		for (size_t i = 0; i < texture_animations.size(); i++) {
			if (!used_texture_animations.contains(static_cast<uint32_t>(i))) {
				unused(std::format("Texture animation {} is never referenced", i));
			}
		}

		return messages;
	}

	/// Fixes errors in models that the game tolerates
	void MDX::fix_up() {
		// Remove geoset animations that reference non-existing geosets
		for (size_t i = animations.size(); i-- > 0;) {
			if (animations[i].geoset_id >= geosets.size()) {
				animations.erase(animations.begin() + i);
			}
		}

		size_t node_count = bones.size() + lights.size() + help_bones.size() + attachments.size() + emitters1.size() + emitters2.size()
			+ ribbons.size() + event_objects.size() + collision_shapes.size() + corn_emitters.size();

		// If there are no bones we have to add one to prevent crashing and stuff.
		if (bones.empty()) {
			Bone bone {};
			bone.node.parent_id = -1;
			bone.node.id = node_count++;
			bones.push_back(bone);
		}

		// ———————————No sequences?———————————
		// ⣞⢽⢪⢣⢣⢣⢫⡺⡵⣝⡮⣗⢷⢽⢽⢽⣮⡷⡽⣜⣜⢮⢺⣜⢷⢽⢝⡽⣝
		//⠸⡸⠜⠕⠕⠁⢁⢇⢏⢽⢺⣪⡳⡝⣎⣏⢯⢞⡿⣟⣷⣳⢯⡷⣽⢽⢯⣳⣫⠇
		//⠀⠀⢀⢀⢄⢬⢪⡪⡎⣆⡈⠚⠜⠕⠇⠗⠝⢕⢯⢫⣞⣯⣿⣻⡽⣏⢗⣗⠏⠀
		//⠀⠪⡪⡪⣪⢪⢺⢸⢢⢓⢆⢤⢀⠀⠀⠀⠀⠈⢊⢞⡾⣿⡯⣏⢮⠷⠁⠀⠀
		//⠀⠀⠀⠈⠊⠆⡃⠕⢕⢇⢇⢇⢇⢇⢏⢎⢎⢆⢄⠀⢑⣽⣿⢝⠲⠉⠀⠀⠀⠀
		//⠀⠀⠀⠀⠀⡿⠂⠠⠀⡇⢇⠕⢈⣀⠀⠁⠡⠣⡣⡫⣂⣿⠯⢪⠰⠂⠀⠀⠀⠀
		//⠀⠀⠀⠀⡦⡙⡂⢀⢤⢣⠣⡈⣾⡃⠠⠄⠀⡄⢱⣌⣶⢏⢊⠂⠀⠀⠀⠀⠀⠀
		//⠀⠀⠀⠀⢝⡲⣜⡮⡏⢎⢌⢂⠙⠢⠐⢀⢘⢵⣽⣿⡿⠁⠁⠀⠀⠀⠀⠀⠀⠀
		//⠀⠀⠀⠀⠨⣺⡺⡕⡕⡱⡑⡆⡕⡅⡕⡜⡼⢽⡻⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
		//⠀⠀⠀⠀⣼⣳⣫⣾⣵⣗⡵⡱⡡⢣⢑⢕⢜⢕⡝⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
		//⠀⠀⠀⣴⣿⣾⣿⣿⣿⡿⡽⡑⢌⠪⡢⡣⣣⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
		//⠀⠀⠀⡟⡾⣿⢿⢿⢵⣽⣾⣼⣘⢸⢸⣞⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
		//⠀⠀⠀⠀⠁⠇⠡⠩⡫⢿⣝⡻⡮⣒⢽⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
		//————————————————————————————————————
		if (sequences.empty()) {
			sequences.push_back(
				Sequence {
					.name = "stand",
					.start_frame = 0,
					.end_frame = 0,
					.movespeed = 0.f,
					.flags = Sequence::Flags::looping,
					.rarity = 0.f,
					.sync_point = 0,
					.extent = extent
				}
			);
		}

		// Can't have zero-sized extents unless you're rendering nothing!
		if (extent.minimum == glm::vec3(0.f) && extent.maximum == glm::vec3(0.f)) {
			calculate_extents();
		}

		// Sometimes (for doodads mostly) the sequence extents are empty (0.0) which messes with culling
		for (auto& i : sequences) {
			if (i.extent.minimum == glm::vec3(0.f) && i.extent.maximum == glm::vec3(0.f)) {
				i.extent = extent;
			}
		}

		// We rely on sortedness for MDX optimization
		std::ranges::sort(sequences, [](const auto& a, const auto& b) {
			return a.start_frame < b.start_frame;
		});

		// Ensure that the pivot buffer is big enough
		pivots.resize(node_count, {});

		// Compact node IDs
		std::vector<int> IDs;
		IDs.reserve(node_count);
		for_each_node([&](const Node& node) {
			IDs.push_back(node.id);
		});

		const int max_id = *std::max_element(IDs.begin(), IDs.end());
		std::vector<int> remapping(max_id + 1);
		for (size_t i = 0; i < IDs.size(); i++) {
			remapping[IDs[i]] = i;
		}

		for_each_node([&](mdx::Node& node) {
			node.id = remapping[node.id];
			if (node.parent_id != -1) {
				node.parent_id = remapping[node.parent_id];
			}
		});

		// Fix vertex groups that reference non-existent matrix groups
		for (auto& i : geosets) {
			// RMS seems to output -1 here sometimes ;(
			if (i.lod == std::numeric_limits<uint32_t>::max()) {
				i.lod = 0;
			}
			for (auto& j : i.vertex_groups) {
				// If no matrix groups exist, we insert one
				if (i.matrix_groups.empty()) {
					i.matrix_groups.push_back(1);
					i.matrix_indices.push_back(0);
				}
				// Don't reference non-existing ones!
				if (j >= i.matrix_groups.size()) {
					j = std::min<uint8_t>(j, i.matrix_groups.size() - 1);
				}
			}
		}
	}
} // namespace mdx
