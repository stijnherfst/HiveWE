#include "MDX.h"

#include "Utilities.h"
#include <iostream>
#include <bit>

namespace mdx {
	std::map<int, std::string> replacable_id_to_texture{
		{ 1, "ReplaceableTextures/TeamColor/TeamColor00.dds" },
		{ 2, "ReplaceableTextures/TeamGlow/TeamGlow00.dds" },
		{ 11, "ReplaceableTextures/Cliff/Cliff0.dds" },
		{ 31, "ReplaceableTextures/LordaeronTree/LordaeronFallTree.dds" },
		{ 32, "ReplaceableTextures/AshenvaleTree/AshenTree.dds" },
		{ 33, "ReplaceableTextures/BarrensTree/BarrensTree.dds" },
		{ 34, "ReplaceableTextures/NorthrendTree/NorthTree.dds" },
		{ 35, "ReplaceableTextures/Mushroom/MushroomTree.dds" },
		{ 36, "ReplaceableTextures/RuinsTree/RuinsTree.dds" },
		{ 37, "ReplaceableTextures/OutlandMushroomTree/MushroomTree.dds" }
	};

	Extent::Extent(BinaryReader& reader) {
		bounds_radius = reader.read<float>();
		minimum = reader.read<glm::vec3>();
		maximum = reader.read<glm::vec3>();
	}

	void Extent::save(BinaryWriter& writer) const {
		writer.write<float>(bounds_radius);
		writer.write<glm::vec3>(minimum);
		writer.write<glm::vec3>(maximum);
	}


	Node::Node(BinaryReader& reader, int& unique_tracks) {
		const size_t reader_pos = reader.position;
		const uint32_t inclusive_size = reader.read<uint32_t>();
		name = reader.read_string(80);
		id = reader.read<uint32_t>();
		parent_id = reader.read<uint32_t>();
		flags = reader.read<uint32_t>();

		while (reader.position < reader_pos + inclusive_size) {
			TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());
			if (tag == TrackTag::KGTR) {
				KGTR = TrackHeader<glm::vec3>(reader, unique_tracks++);
			} else if (tag == TrackTag::KGRT) {
				KGRT = TrackHeader<glm::quat>(reader, unique_tracks++);
			} else if (tag == TrackTag::KGSC) {
				KGSC = TrackHeader<glm::vec3>(reader, unique_tracks++);
			} else {
				std::cout << "Unknown track tag\n";
			}
		}
	}

	void MDX::validate() {
		// Remove geoset animations that reference non existing geosets
		for (size_t i = animations.size(); i-- > 0;) {
			if (animations[i].geoset_id >= geosets.size()) {
				animations.erase(animations.begin() + i);
			}
		}

		// Fix vertex groups that reference non existent bone groups
		for (auto& i : geosets) {
			for (auto& j : i.vertex_groups) {
				if (i.bone_groups.empty()) {
					// Do something like referencing an existing bone. I dunno
				}
				if (j >= i.bone_groups.size()) {
					j = std::min<uint8_t>(j, i.bone_groups.size() - 1);
				}
			}
		}
	}

	void MDX::read_GEOS_chunk(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		uint32_t total_size = 0;

		while (total_size < size) {
			total_size += reader.read<uint32_t>();

			Geoset geoset;
			reader.advance(4);
			const uint32_t vertex_count = reader.read<uint32_t>();
			geoset.vertices = reader.read_vector<glm::vec3>(vertex_count);
			reader.advance(4);
			const uint32_t normal_count = reader.read<uint32_t>();
			geoset.normals = reader.read_vector<glm::vec3>(normal_count);
			reader.advance(4);
			const uint32_t face_type_groups_count = reader.read<uint32_t>();
			geoset.face_type_groups = reader.read_vector<uint32_t>(face_type_groups_count);
			reader.advance(4);
			const uint32_t face_groups_count = reader.read<uint32_t>();
			geoset.face_groups = reader.read_vector<uint32_t>(face_groups_count);
			reader.advance(4);
			const uint32_t faces_count = reader.read<uint32_t>();
			geoset.faces = reader.read_vector<uint16_t>(faces_count);
			reader.advance(4);
			const uint32_t vertex_groups_count = reader.read<uint32_t>();
			geoset.vertex_groups = reader.read_vector<uint8_t>(vertex_groups_count);
			reader.advance(4);
			const uint32_t matrix_group_count = reader.read<uint32_t>();
			geoset.bone_groups = reader.read_vector<uint32_t>(matrix_group_count);
			reader.advance(4); // Mats
			const uint32_t matrix_indices_count = reader.read<uint32_t>();
			geoset.bone_indices = reader.read_vector<uint32_t>(matrix_indices_count);
			geoset.material_id = reader.read<uint32_t>();
			geoset.selection_group = reader.read<uint32_t>();
			geoset.selection_flags = reader.read<uint32_t>();

			if (version > 800) {
				geoset.lod = reader.read<uint32_t>();
				geoset.lod_name = reader.read_string(80); // lod name
			} else {
				geoset.lod = 0;
			}

			geoset.extent = Extent(reader);
			const uint32_t extents_count = reader.read<uint32_t>();
			for (size_t i = 0; i < extents_count; i++) {
				geoset.extents.emplace_back(Extent(reader));
			}

			std::string tag = reader.read_string(4);

			if (tag == "TANG") {
				uint32_t structure_count = reader.read<uint32_t>();
				geoset.tangents = reader.read_vector<glm::vec4>(structure_count);
				tag = reader.read_string(4); // Maybe SKIN, maybe UVAS
			}

			if (tag == "SKIN") {
				uint32_t skin_count = reader.read<uint32_t>();
				geoset.skin = reader.read_vector<uint8_t>(skin_count);
				reader.advance(4); // UVAS
			}

			const uint32_t texture_coordinate_sets_count = reader.read<uint32_t>();
			for (size_t i = 0; i < texture_coordinate_sets_count; i++) {
				reader.advance(4);
				const uint32_t texture_coordinates_count = reader.read<uint32_t>();
				geoset.texture_coordinate_sets.push_back(reader.read_vector<glm::vec2>(texture_coordinates_count));
			}

			geosets.push_back(std::move(geoset));
		}
	}

	void MDX::read_MTLS_chunk(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		uint32_t total_size = 0;

		while (total_size < size) {
			total_size += reader.read<uint32_t>();

			Material material;
			material.priority_plane = reader.read<uint32_t>();
			material.flags = reader.read<uint32_t>();
			if (version > 800) {
				material.shader_name = reader.read_string(80);
			}
			reader.advance(4);
			const uint32_t layers_count = reader.read<uint32_t>();

			for (size_t i = 0; i < layers_count; i++) {
				const size_t reader_pos = reader.position;
				Layer layer;
				const uint32_t size = reader.read<uint32_t>();
				layer.blend_mode = reader.read<uint32_t>();
				layer.shading_flags = reader.read<uint32_t>();
				layer.texture_id = reader.read<uint32_t>();
				layer.texture_animation_id = reader.read<uint32_t>();
				layer.coord_id = reader.read<uint32_t>();
				layer.alpha = reader.read<float>();

				if (version > 800) {
					layer.emissive_gain = reader.read<float>();
					layer.fresnel_color = reader.read<glm::vec3>();
					layer.fresnel_opacity = reader.read<float>();
					layer.fresnel_team_color = reader.read<float>();
				}

				while (reader.position < reader_pos + size) {
					TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());
					if (tag == TrackTag::KMTF) {
						layer.KMTF = TrackHeader<uint32_t>(reader, unique_tracks++);
					} else if (tag == TrackTag::KMTA) {
						layer.KMTA = TrackHeader<float>(reader, unique_tracks++);
					} else if (tag == TrackTag::KMTE) {
						layer.KMTE = TrackHeader<float>(reader, unique_tracks++);
					} else if (tag == TrackTag::KFC3) {
						layer.KFC3 = TrackHeader<glm::vec3>(reader, unique_tracks++);
					} else if (tag == TrackTag::KFCA) {
						layer.KFCA = TrackHeader<float>(reader, unique_tracks++);
					} else if (tag == TrackTag::KFTC) {
						layer.KFTC = TrackHeader<float>(reader, unique_tracks++);
					} else {
						std::cout << "Unknown track tag\n";
					}
				}

				material.layers.push_back(std::move(layer));
			}

			materials.push_back(std::move(material));
		}
	}

	void MDX::read_SEQS_chunk(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		for (size_t i = 0; i < size / 132; i++) {
			Sequence sequence;
			sequence.name = reader.read_string(80);
			sequence.start_frame = reader.read<uint32_t>();
			sequence.end_frame = reader.read<uint32_t>();
			sequence.movespeed = reader.read<float>();
			sequence.flags = reader.read<uint32_t>();
			sequence.rarity = reader.read<float>();
			sequence.sync_point = reader.read<uint32_t>();
			sequence.extent = Extent(reader);
			sequences.push_back(std::move(sequence));
		}
	}

	void MDX::read_GEOA_chunk(BinaryReader& reader) {
		uint32_t remaining_size = reader.read<uint32_t>();

		while (remaining_size > 0) {
			const size_t reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			remaining_size -= inclusive_size;

			GeosetAnimation animation;
			animation.alpha = reader.read<float>();
			animation.flags = reader.read<uint32_t>();
			animation.color = reader.read<glm::vec3>();
			animation.geoset_id = reader.read<uint32_t>();

			while (reader.position < reader_pos + inclusive_size) {
				TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());
				if (tag == TrackTag::KGAO) {
					animation.KGAO = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KGAC) {
					animation.KGAC = TrackHeader<glm::vec3>(reader, unique_tracks++);
				} else {
					std::cout << "Unknown track tag\n";
				}
			}

			animations.push_back(std::move(animation));
		}
	}

	void MDX::read_BONE_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Bone bone;
			bone.node = Node(reader, unique_tracks);
			bone.geoset_id = reader.read<int32_t>();
			bone.geoset_animation_id = reader.read<int32_t>();
			bones.push_back(std::move(bone));
		}
	}

	void MDX::read_TEXS_chunk(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		for (size_t i = 0; i < size / 268; i++) {
			Texture texture;
			texture.replaceable_id = reader.read<uint32_t>();
			texture.file_name = reader.read_string(260);
			texture.flags = reader.read<uint32_t>();
			textures.push_back(std::move(texture));
		}
	}

	void MDX::read_GLBS_chunk(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		global_sequences = reader.read_vector<uint32_t>(size / 4);
	}

	void MDX::read_LITE_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Light light;
			const size_t node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			light.node = Node(reader, unique_tracks);
			light.type = reader.read<uint32_t>();
			light.attenuation_start = reader.read<float>();
			light.attenuation_end = reader.read<float>();
			light.color = reader.read<glm::vec3>();
			light.intensity = reader.read<float>();
			light.ambient_color = reader.read<glm::vec3>();
			light.ambient_intensity = reader.read<float>();
			while (reader.position < node_reader_pos + inclusive_size) {
				TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());
				if (tag == TrackTag::KLAS) {
					light.KLAS = TrackHeader<uint32_t>(reader, unique_tracks++);
				} else if (tag == TrackTag::KLAE) {
					light.KLAE = TrackHeader<uint32_t>(reader, unique_tracks++);
				} else if (tag == TrackTag::KLAC) {
					light.KLAC = TrackHeader<glm::vec3>(reader, unique_tracks++);
				} else if (tag == TrackTag::KLAI) {
					light.KLAI = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KLBI) {
					light.KLBI = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KLBC) {
					light.KLBC = TrackHeader<glm::vec3>(reader, unique_tracks++);
				} else if (tag == TrackTag::KLAV) {
					light.KLAV = TrackHeader<float>(reader, unique_tracks++);
				} else {
					std::cout << "Unknown track tag\n";
				}
			}
			lights.push_back(std::move(light));
		}
	}

	void MDX::read_HELP_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();
		while (reader.position < reader_pos + size) {
			help_bones.push_back(Node(reader, unique_tracks));
		}
	}

	void MDX::read_ATCH_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Attachment attachment;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			attachment.node = Node(reader, unique_tracks);
			attachment.path = reader.read_string(256);
			attachment.reserved = reader.read<uint32_t>();
			attachment.attachment_id = reader.read<uint32_t>();
			while (reader.position < node_reader_pos + inclusive_size) {
				TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());
				attachment.KATV = TrackHeader<float>(reader, unique_tracks++);
				if (tag != TrackTag::KATV) {
					std::cout << "Unknown track tag\n";
				}
			}
			attachments.push_back(std::move(attachment));
		}
	}

	void MDX::read_PIVT_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		pivots = reader.read_vector<glm::vec3>(size / 12);
	}

	void MDX::read_PREM_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			ParticleEmitter1 emitter;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter.node = Node(reader, unique_tracks);
			emitter.emission_rate = reader.read<float>();
			emitter.gravity = reader.read<float>();
			emitter.longitude = reader.read<float>();
			emitter.latitude = reader.read<float>();
			emitter.path = reader.read_string(256);
			emitter.reserved = reader.read<uint32_t>();
			emitter.life_span = reader.read<float>();
			emitter.speed = reader.read<float>();
			while (reader.position < node_reader_pos + inclusive_size) {
				TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());
				if (tag == TrackTag::KPEE) {
					emitter.KPEE = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KPEG) {
					emitter.KPEG = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KPLN) {
					emitter.KPLN = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KPLT) {
					emitter.KPLT = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KPEL) {
					emitter.KPEL = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KPES) {
					emitter.KPES = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KPEV) {
					emitter.KPEV = TrackHeader<float>(reader, unique_tracks++);
				} else {
					std::cout << "Unknown track tag\n";
				}
			}
			emitters1.push_back(std::move(emitter));
		}
	}

	void MDX::read_PRE2_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			ParticleEmitter2 emitter2;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter2.node = Node(reader, unique_tracks);

			emitter2.speed = reader.read<float>();
			emitter2.variation = reader.read<float>();
			emitter2.latitude = reader.read<float>();
			emitter2.gravity = reader.read<float>();
			emitter2.life_span = reader.read<float>();
			emitter2.emission_rate = reader.read<float>();
			emitter2.length = reader.read<float>();
			emitter2.width = reader.read<float>();
			emitter2.filter_mode = reader.read<uint32_t>();
			emitter2.rows = reader.read<uint32_t>();
			emitter2.columns = reader.read<uint32_t>();
			emitter2.head_or_tail = reader.read<uint32_t>();
			emitter2.tail_length = reader.read<float>();
			emitter2.time_middle = reader.read<float>();
			for (int time = 0; time < 3; time++) {
				for (int i = 0; i < 3; i++) {
					emitter2.segment_color[time][i] = reader.read<float>();
				}
			}
			for (int i = 0; i < 3; i++) {
				emitter2.segment_alphas[i] = reader.read<uint8_t>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.segment_scaling[i] = reader.read<float>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.head_intervals[i] = reader.read<float>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.head_decay_intervals[i] = reader.read<float>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.tail_intervals[i] = reader.read<float>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.tail_decay_intervals[i] = reader.read<float>();
			}
			emitter2.texture_id = reader.read<uint32_t>();
			emitter2.squirt = reader.read<uint32_t>();
			emitter2.priority_plane = reader.read<uint32_t>();
			emitter2.replaceable_id = reader.read<uint32_t>();

			while (reader.position < node_reader_pos + inclusive_size) {
				TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());
				if (tag == TrackTag::KP2S) {
					emitter2.KP2S = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KP2R) {
					emitter2.KP2R = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KP2L) {
					emitter2.KP2L = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KP2G) {
					emitter2.KP2G = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KP2E) {
					emitter2.KP2E = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KP2N) {
					emitter2.KP2N = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KP2W) {
					emitter2.KP2W = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KP2V) {
					emitter2.KP2V = TrackHeader<float>(reader, unique_tracks++);
				} else {
					std::cout << "Unknown track tag\n";
				}
			}
			emitters2.push_back(std::move(emitter2));
		}
	}

	void MDX::read_RIBB_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			RibbonEmitter emitter;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter.node = Node(reader, unique_tracks);
			emitter.height_above = reader.read<float>();
			emitter.height_below = reader.read<float>();
			emitter.alpha = reader.read<float>();
			emitter.color = reader.read<glm::vec3>();
			emitter.life_span = reader.read<float>();
			emitter.texture_slot = reader.read<uint32_t>();
			emitter.emission_rate = reader.read<uint32_t>();
			emitter.rows = reader.read<uint32_t>();
			emitter.columns = reader.read<uint32_t>();
			emitter.material_id = reader.read<uint32_t>();
			emitter.gravity = reader.read<float>();
			while (reader.position < node_reader_pos + inclusive_size) {
				TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());
				if (tag == TrackTag::KRHA) {
					emitter.KRHA = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KRHB) {
					emitter.KRHB = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KRAL) {
					emitter.KRAL = TrackHeader<float>(reader, unique_tracks++);
				} else if (tag == TrackTag::KRCO) {
					emitter.KRCO = TrackHeader<glm::vec3>(reader, unique_tracks++);
				} else if (tag == TrackTag::KRTX) {
					emitter.KRTX = TrackHeader<uint32_t>(reader, unique_tracks++);
				} else if (tag == TrackTag::KRVS) {
					emitter.KRVS = TrackHeader<float>(reader, unique_tracks++);
				} else {
					std::cout << "Unknown track tag\n";
				}
			}
			ribbons.push_back(std::move(emitter));
		}
	}

	void MDX::read_EVTS_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			EventObject evt;
			evt.node = Node(reader, unique_tracks);
			reader.read<uint32_t>(); // read KEVT
			evt.count = reader.read<uint32_t>();
			evt.global_sequence_id = reader.read<int32_t>(); //signed
			for (int i = 0; i < evt.count; i++) {
				evt.times.push_back(reader.read<uint32_t>());
			}
			eventObjects.push_back(std::move(evt));
		}
	}

	void MDX::read_CLID_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			CollisionShape shape;
			shape.node = Node(reader, unique_tracks);

			uint32_t type_index = reader.read<uint32_t>();
			switch (type_index) {
				case 1:
					shape.type = CollisionShapeType::Plane;
					break;
				case 2:
					shape.type = CollisionShapeType::Sphere;
					break;
				case 3:
					shape.type = CollisionShapeType::Cylinder;
					break;
				default:
				case 0:
					shape.type = CollisionShapeType::Box;
					break;
			}

			for (int i = 0; i < 3; i++) {
				if (reader.remaining() <= 0) {
					shape.vertices[0][i] = 0;
				} else {
					shape.vertices[0][i] = reader.read<float>();
				}
			}

			if (type_index != 2) {
				for (int i = 0; i < 3; i++) {
					if (reader.remaining() <= 0) {
						shape.vertices[1][i] = 0;
					} else {
						shape.vertices[1][i] = reader.read<float>();
					}
				}
			}

			if (type_index == 2 || type_index == 3) {
				if (reader.remaining() > 0) {
					shape.radius = reader.read<float>();
				} else {
					shape.radius = 0;
				}
			}
			collisionShapes.push_back(std::move(shape));
		}
	}

	void MDX::read_CORN_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			CornEmitter emitter;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter.node = Node(reader, unique_tracks);

			reader.advance(4);
			reader.advance(4);
			reader.advance(4);
			reader.advance(16);
			reader.advance(4);
			reader.advance(260);
			reader.advance(260);

			reader.advance(inclusive_size - (reader.position - node_reader_pos));

			//while (reader.position < node_reader_pos + inclusive_size) {
			//	emitter.node.animated_data.load_tracks(reader);
			//}
			corn_emitters.push_back(std::move(emitter));
		}
	}

	void MDX::write_GEOS_chunk(BinaryWriter& writer) const {
		writer.write(ChunkTag::GEOS);
		// Write temporary zero, remember location
		int inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& geoset : geosets) {
			const int geoset_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			writer.write_string("VRTX");
			writer.write<uint32_t>(geoset.vertices.size());
			writer.write_vector(geoset.vertices);

			writer.write_string("NRMS");
			writer.write<uint32_t>(geoset.normals.size());
			writer.write_vector(geoset.normals);

			writer.write_string("PTYP");
			writer.write<uint32_t>(geoset.face_type_groups.size());
			writer.write_vector(geoset.face_type_groups);

			writer.write_string("PCNT");
			writer.write<uint32_t>(geoset.face_groups.size());
			writer.write_vector(geoset.face_groups);

			writer.write_string("PVTX");
			writer.write<uint32_t>(geoset.faces.size());
			writer.write_vector(geoset.faces);

			writer.write_string("GNDX");
			writer.write<uint32_t>(geoset.vertex_groups.size());
			writer.write_vector(geoset.vertex_groups);

			writer.write_string("MTGC");
			writer.write<uint32_t>(geoset.bone_groups.size());
			writer.write_vector(geoset.bone_groups);

			writer.write_string("MATS");
			writer.write<uint32_t>(geoset.bone_indices.size());
			writer.write_vector(geoset.bone_indices);

			writer.write<uint32_t>(geoset.material_id);
			writer.write<uint32_t>(geoset.selection_group);
			writer.write<uint32_t>(geoset.selection_flags);
			writer.write<uint32_t>(geoset.lod);
			writer.write_c_string_padded(geoset.lod_name, 80);

			geoset.extent.save(writer);
			writer.write<uint32_t>(geoset.extents.size());
			for (const auto& extent : geoset.extents) {
				extent.save(writer);
			}

			writer.write_string("TANG");
			writer.write<uint32_t>(geoset.tangents.size());
			writer.write_vector(geoset.tangents);

			writer.write_string("SKIN");
			writer.write<uint32_t>(geoset.skin.size());
			writer.write_vector(geoset.skin);

			writer.write_string("UVAS");
			writer.write<uint32_t>(geoset.texture_coordinate_sets.size());
			for (const auto& set : geoset.texture_coordinate_sets) {
				writer.write_string("UVBS");
				writer.write<uint32_t>(set.size());
				writer.write_vector(set);
			}

			const uint32_t temporary = writer.buffer.size() - geoset_index;
			std::memcpy(writer.buffer.data() + geoset_index, &temporary, 4);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_MTLS_chunk(BinaryWriter& writer) const {
		writer.write(ChunkTag::MTLS);
		// Write temporary zero, remember location
		int inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& material : materials) {
			// Write temporary zero, remember location
			const int material_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			writer.write<uint32_t>(material.priority_plane);
			writer.write<uint32_t>(material.flags);
			writer.write_c_string_padded(material.shader_name, 80);
			writer.write_string("LAYS");
			writer.write<uint32_t>(material.layers.size());

			for (const auto& layer : material.layers) {
				// Write temporary zero, remember location
				const int layer_index = writer.buffer.size();
				writer.write<uint32_t>(0);

				writer.write<uint32_t>(layer.blend_mode);
				writer.write<uint32_t>(layer.shading_flags);
				writer.write<uint32_t>(layer.texture_id);
				writer.write<uint32_t>(layer.texture_animation_id);
				writer.write<uint32_t>(layer.coord_id);
				writer.write<uint32_t>(layer.alpha);

				writer.write<float>(layer.emissive_gain);
				writer.write<glm::vec3>(layer.fresnel_color);
				writer.write<float>(layer.fresnel_opacity);
				writer.write<float>(layer.fresnel_team_color);

				layer.KMTF.save(writer);
				layer.KMTA.save(writer);
				layer.KMTE.save(writer);
				layer.KFC3.save(writer);
				layer.KFCA.save(writer);
				layer.KFTC.save(writer);
				
				const uint32_t temporary = writer.buffer.size() - layer_index;
				std::memcpy(writer.buffer.data() + layer_index, &temporary, 4);
			}
			const uint32_t temporary = writer.buffer.size() - material_index;
			std::memcpy(writer.buffer.data() + material_index, &temporary, 4);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_SEQS_chunk(BinaryWriter& writer) const {
		writer.write(ChunkTag::SEQS);
		writer.write(sequences.size() * 132);
		for (const auto& i : sequences) {
			writer.write_c_string_padded(i.name, 80);
			writer.write<uint32_t>(i.start_frame);
			writer.write<uint32_t>(i.end_frame);
			writer.write<float>(i.movespeed);
			writer.write<uint32_t>(i.flags);
			writer.write<float>(i.rarity);
			writer.write<uint32_t>(i.sync_point);
		}
	}

	void MDX::write_GLBS_chunk(BinaryWriter& writer) const {
		writer.write(ChunkTag::GLBS);
		writer.write<uint32_t>(global_sequences.size() * 4);
		writer.write_vector(global_sequences);
	}

	void MDX::forEachNode(const std::function<void(Node&)>& F) {
		for (auto& i : bones) {
			F(i.node);
		}

		for (auto& i : lights) {
			F(i.node);
		}

		for (auto& i : help_bones) {
			F(i);
		}

		for (auto& i : attachments) {
			F(i.node);
		}

		for (auto& i : emitters1) {
			F(i.node);
		}

		for (auto& i : emitters2) {
			F(i.node);
		}

		for (auto& i : ribbons) {
			F(i.node);
		}

		for (auto& i : eventObjects) {
			F(i.node);
		}

		for (auto& i : collisionShapes) {
			F(i.node);
		}
		for (auto& i : corn_emitters) {
			F(i.node);
		}
	}

	MDX::MDX(BinaryReader& reader) {
		load(reader);
	}

	void MDX::load(BinaryReader& reader) {
		const std::string magic_number = reader.read_string(4);
		if (magic_number != "MDLX") {
			std::cout << "The file's magic number is incorrect. Should be MDLX, is: " << magic_number << std::endl;
			return;
		}

		while (reader.remaining() > 0) {
			uint32_t header = reader.read<uint32_t>();

			switch (static_cast<ChunkTag>(header)) {
				case ChunkTag::VERS:
					reader.advance(4);
					version = reader.read<uint32_t>();
					break;
				case ChunkTag::GEOS:
					read_GEOS_chunk(reader);
					break;
				case ChunkTag::MTLS:
					read_MTLS_chunk(reader);
					break;
				case ChunkTag::SEQS:
					read_SEQS_chunk(reader);
					break;
				case ChunkTag::GLBS:
					read_GLBS_chunk(reader);
					break;
				case ChunkTag::GEOA:
					read_GEOA_chunk(reader);
					break;
				case ChunkTag::BONE:
					read_BONE_chunk(reader);
					break;
				case ChunkTag::TEXS:
					read_TEXS_chunk(reader);
					break;
				case ChunkTag::LITE:
					read_LITE_chunk(reader);
					break;
				case ChunkTag::HELP:
					read_HELP_chunk(reader);
					break;
				case ChunkTag::ATCH:
					read_ATCH_chunk(reader);
					break;
				case ChunkTag::PIVT:
					read_PIVT_chunk(reader);
					break;
				case ChunkTag::PREM:
					read_PREM_chunk(reader);
					break;
				case ChunkTag::PRE2:
					read_PRE2_chunk(reader);
					break;
				case ChunkTag::RIBB:
					read_RIBB_chunk(reader);
					break;
				case ChunkTag::EVTS:
					read_EVTS_chunk(reader);
					break;
				case ChunkTag::CLID:
					read_CLID_chunk(reader);
					break;
				case ChunkTag::CORN:
					read_CORN_chunk(reader);
					break;
				default:
					reader.advance(reader.read<uint32_t>());
			}
		}

		validate();
	}

	void MDX::save(const fs::path& path) {
		BinaryWriter writer;

		writer.write_string("MDLX");
		writer.write(ChunkTag::VERS);
		writer.write<uint32_t>(4);
		writer.write<uint32_t>(1000);

		write_SEQS_chunk(writer);
		write_GLBS_chunk(writer);
	}
} // namespace mdx