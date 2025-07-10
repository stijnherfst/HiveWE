module;

module MDX;

import std;

namespace mdx {
	void MDX::validate() {
		// Remove geoset animations that reference non-existing geosets
		for (size_t i = animations.size(); i-- > 0;) {
			if (animations[i].geoset_id >= geosets.size()) {
				animations.erase(animations.begin() + i);
			}
		}

		size_t node_count = bones.size() +
							lights.size() +
							help_bones.size() +
							attachments.size() +
							emitters1.size() +
							emitters2.size() +
							ribbons.size() +
							event_objects.size() +
							collision_shapes.size() +
							corn_emitters.size();

		// If there are no bones we have to add one to prevent crashing and stuff.
		if (bones.empty()) {
			Bone bone{};
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
			sequences.push_back(Sequence {
				.name = "stand",
				.start_frame = 0,
				.end_frame = 0,
				.movespeed = 0.f,
				.flags = Sequence::Flags::looping,
				.rarity = 0.f,
				.sync_point = 0,
				.extent = extent
			});
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
		for_each_node([&](mdx::Node& node) {
			if (node.id < 0) {
				std::println("Error: MDX {} node \"{}\" has invalid ID {}", name, node.name, node.id);
				return;
			}
			IDs.push_back(node.id);
		});

		const int max_id = *std::max_element(IDs.begin(), IDs.end());
		std::vector<int> remapping(max_id + 1);
		for (size_t i = 0; i < IDs.size(); i++) {
			remapping[IDs[i]] = i;
		}

		for_each_node([&](mdx::Node& node) {
			if (node.id == -1) {
				std::println("Error: Invalid node \"{}\" with ID -1", node.name);
				return;
			}
			node.id = remapping[node.id];
			if (node.parent_id != -1) {
				node.parent_id = remapping[node.parent_id];
			}
		});

		for (auto& i : geosets) {
			if (i.uv_sets.empty()) {
				std::println("Error: No UV sets in model");
				return;
			}

			if (!i.vertex_groups.empty() && !i.skin.empty()) {
				std::println("Error: Both vertex_groups and skin weights are set");
				return;
			}

			bool same = i.vertices.size() == i.uv_sets.front().size() && i.vertices.size() == i.normals.size();
			if (i.vertex_groups.size() > 0) {
				same = same && i.vertices.size() == i.vertex_groups.size();

				if (!same) {
					std::println("One or more of these are inequal.\nvertices: {}\nuv_sets: {}\nnormals: {}\nvertex_groups: {}", i.vertices.size(), i.uv_sets.front().size(), i.normals.size(), i.vertex_groups.size());
					return;
				}
			} else {
				same = same && i.vertices.size() == i.skin.size() / 8;

				if (!same) {
					std::println("One or more of these are inequal.\nvertices: {}\nuv_sets: {}\nnormals: {}\nskin weights: {}", i.vertices.size(), i.uv_sets.front().size(), i.normals.size(), i.skin.size() / 8);
					return;
				}
			}

			for (const auto& set : i.uv_sets) {
				if (set.empty()) {
					std::println("Error: Empty UV set");
					return;
				}
			}
		}

		// Fix vertex groups that reference non existent matrix groups
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