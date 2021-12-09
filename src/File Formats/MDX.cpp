#include "MDX.h"

#include "Utilities.h"
#include <fstream>
#include <functional>
#include <fmt/format.h>

namespace mdx {
	const std::unordered_map<int, std::string> replacable_id_to_texture{
		{ 1, "ReplaceableTextures/TeamColor/TeamColor00" },
		{ 2, "ReplaceableTextures/TeamGlow/TeamGlow00" },
		{ 11, "ReplaceableTextures/Cliff/Cliff0" },
		{ 31, "ReplaceableTextures/LordaeronTree/LordaeronFallTree" },
		{ 32, "ReplaceableTextures/AshenvaleTree/AshenTree" },
		{ 33, "ReplaceableTextures/BarrensTree/BarrensTree" },
		{ 34, "ReplaceableTextures/NorthrendTree/NorthTree" },
		{ 35, "ReplaceableTextures/Mushroom/MushroomTree" },
		{ 36, "ReplaceableTextures/RuinsTree/RuinsTree" },
		{ 37, "ReplaceableTextures/OutlandMushroomTree/MushroomTree" }
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
				fmt::print("Unknown track tag {}\n", tag);
			}
		}
	}

	void Node::save(BinaryWriter& writer) const {
		// Write temporary zero, remember location
		size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		writer.write_c_string_padded(name, 80);
		writer.write<uint32_t>(id);
		writer.write<uint32_t>(parent_id);
		writer.write<uint32_t>(flags);

		KGTR.save(TrackTag::KGTR, writer);
		KGRT.save(TrackTag::KGRT, writer);
		KGSC.save(TrackTag::KGSC, writer);

		const uint32_t temporary = writer.buffer.size() - inclusive_index;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
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
			geoset.matrix_groups = reader.read_vector<uint32_t>(matrix_group_count);
			reader.advance(4); // Mats
			const uint32_t matrix_indices_count = reader.read<uint32_t>();
			geoset.matrix_indices = reader.read_vector<uint32_t>(matrix_indices_count);
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
						fmt::print("Unknown track tag {}\n", tag);
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
					fmt::print("Unknown track tag {}\n", tag);
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
					fmt::print("Unknown track tag {}\n", tag);
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
					fmt::print("Unknown track tag {}\n", tag);
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
					fmt::print("Unknown track tag {}\n", tag);
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

			emitter2.start_segment_color = reader.read<glm::vec3>();
			emitter2.middle_segment_color = reader.read<glm::vec3>();
			emitter2.end_segment_color = reader.read<glm::vec3>();

			emitter2.segment_alphas = reader.read<glm::u8vec3>();
			emitter2.segment_scaling = reader.read<glm::vec3>();
			emitter2.head_intervals = reader.read<glm::uvec3>();
			emitter2.head_decay_intervals = reader.read<glm::uvec3>();
			emitter2.tail_intervals = reader.read<glm::uvec3>();
			emitter2.tail_decay_intervals = reader.read<glm::uvec3>();

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
					fmt::print("Unknown track tag {}\n", tag);
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
					fmt::print("Unknown track tag {}\n", tag);
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
			reader.advance(4); // read KEVT
			uint32_t count = reader.read<uint32_t>();
			evt.global_sequence_id = reader.read<int32_t>(); //signed
			evt.times = reader.read_vector<uint32_t>(count);
			eventObjects.push_back(std::move(evt));
		}
	}

	void MDX::read_CLID_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			CollisionShape shape;
			shape.node = Node(reader, unique_tracks);
			shape.type = static_cast<CollisionShapeType>(reader.read<uint32_t>());

			for (int i = 0; i < 3; i++) {
				if (reader.remaining() <= 0) {
					shape.vertices[0][i] = 0.f;
				} else {
					shape.vertices[0][i] = reader.read<float>();
				}
			}

			if (shape.type != CollisionShapeType::Sphere) {
				for (int i = 0; i < 3; i++) {
					if (reader.remaining() <= 0) {
						shape.vertices[1][i] = 0.f;
					} else {
						shape.vertices[1][i] = reader.read<float>();
					}
				}
			}

			if (shape.type == CollisionShapeType::Sphere || shape.type == CollisionShapeType::Cylinder) {
				if (reader.remaining() > 0) {
					shape.radius = reader.read<float>();
				} else {
					shape.radius = 0.f;
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
			emitter.data = reader.read_vector<uint8_t>(inclusive_size - (reader.position - node_reader_pos));
			corn_emitters.push_back(std::move(emitter));
		}
	}

	void MDX::read_CAMS_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Camera camera;
			const uint32_t inclusive_size = reader.read<uint32_t>();

			camera.data = reader.read_vector<uint8_t>(inclusive_size - 4);
			cameras.push_back(std::move(camera));
		}
	}

	void MDX::read_BPOS_chunk(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		bind_poses = reader.read_vector<float>(reader.read<uint32_t>() * 12);
	}

	void MDX::read_TXAN_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			TextureAnimation animation;
			const uint32_t inclusive_size = reader.read<uint32_t>();

			animation.data = reader.read_vector<uint8_t>(inclusive_size - 4);
			texture_animations.push_back(std::move(animation));
		}
	}

	void MDX::read_FAFX_chunk(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		for (size_t i = 0; i < size / 340; i++) {
			FaceFX facefx;
			facefx.name = reader.read_string(80);
			facefx.path = reader.read_string(260);
			facefxes.push_back(std::move(facefx));
		}
	}

	void MDX::write_GEOS_chunk(BinaryWriter& writer) const {
		if (geosets.empty()) {
			return;
		}

		writer.write(ChunkTag::GEOS);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& geoset : geosets) {
			// Write temporary zero, remember location
			const size_t geoset_index = writer.buffer.size();
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
			writer.write<uint32_t>(geoset.matrix_groups.size());
			writer.write_vector(geoset.matrix_groups);

			writer.write_string("MATS");
			writer.write<uint32_t>(geoset.matrix_indices.size());
			writer.write_vector(geoset.matrix_indices);

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

			if (geoset.tangents.size()) {
				writer.write_string("TANG");
				writer.write<uint32_t>(geoset.tangents.size());
				writer.write_vector(geoset.tangents);
			}

			if (geoset.skin.size()) {
				writer.write_string("SKIN");
				writer.write<uint32_t>(geoset.skin.size());
				writer.write_vector(geoset.skin);
			}

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
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_MTLS_chunk(BinaryWriter& writer) const {
		if (materials.empty()) {
			return;
		}

		writer.write(ChunkTag::MTLS);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& material : materials) {
			// Write temporary zero, remember location
			const size_t material_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			writer.write<uint32_t>(material.priority_plane);
			writer.write<uint32_t>(material.flags);
			writer.write_c_string_padded(material.shader_name, 80);
			writer.write_string("LAYS");
			writer.write<uint32_t>(material.layers.size());

			for (const auto& layer : material.layers) {
				// Write temporary zero, remember location
				const size_t layer_index = writer.buffer.size();
				writer.write<uint32_t>(0);

				writer.write<uint32_t>(layer.blend_mode);
				writer.write<uint32_t>(layer.shading_flags);
				writer.write<uint32_t>(layer.texture_id);
				writer.write<uint32_t>(layer.texture_animation_id);
				writer.write<uint32_t>(layer.coord_id);
				writer.write<float>(layer.alpha);

				writer.write<float>(layer.emissive_gain);
				writer.write<glm::vec3>(layer.fresnel_color);
				writer.write<float>(layer.fresnel_opacity);
				writer.write<float>(layer.fresnel_team_color);

				layer.KMTF.save(TrackTag::KMTF, writer);
				layer.KMTA.save(TrackTag::KMTA, writer);
				layer.KMTE.save(TrackTag::KMTE, writer);
				layer.KFC3.save(TrackTag::KFC3, writer);
				layer.KFCA.save(TrackTag::KFCA, writer);
				layer.KFTC.save(TrackTag::KFTC, writer);
				
				const uint32_t temporary = writer.buffer.size() - layer_index;
				std::memcpy(writer.buffer.data() + layer_index, &temporary, 4);
			}
			const uint32_t temporary = writer.buffer.size() - material_index;
			std::memcpy(writer.buffer.data() + material_index, &temporary, 4);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_SEQS_chunk(BinaryWriter& writer) const {
		if (sequences.empty()) {
			return;
		}

		writer.write(ChunkTag::SEQS);
		writer.write<uint32_t>(sequences.size() * 132);
		for (const auto& i : sequences) {
			writer.write_c_string_padded(i.name, 80);
			writer.write<uint32_t>(i.start_frame);
			writer.write<uint32_t>(i.end_frame);
			writer.write<float>(i.movespeed);
			writer.write<uint32_t>(i.flags);
			writer.write<float>(i.rarity);
			writer.write<uint32_t>(i.sync_point);
			i.extent.save(writer);
		}
	}

	void MDX::write_GLBS_chunk(BinaryWriter& writer) const {
		if (global_sequences.empty()) {
			return;
		}

		writer.write(ChunkTag::GLBS);
		writer.write<uint32_t>(global_sequences.size() * 4);
		writer.write_vector(global_sequences);
	}

	void MDX::write_GEOA_chunk(BinaryWriter& writer) const {
		if (animations.empty()) {
			return;
		}

		writer.write(ChunkTag::GEOA);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& geoset_animation : animations) {
			// Write temporary zero, remember location
			const size_t geoset_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			writer.write<float>(geoset_animation.alpha);
			writer.write<uint32_t>(geoset_animation.flags);
			writer.write<glm::vec3>(geoset_animation.color);
			writer.write<uint32_t>(geoset_animation.geoset_id);

			geoset_animation.KGAO.save(TrackTag::KGAO, writer);
			geoset_animation.KGAC.save(TrackTag::KGAC, writer);

			const uint32_t temporary = writer.buffer.size() - geoset_index;
			std::memcpy(writer.buffer.data() + geoset_index, &temporary, 4);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_BONE_chunk(BinaryWriter& writer) const {
		if (bones.empty()) {
			return;
		}

		writer.write(ChunkTag::BONE);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& bone : bones) {
			bone.node.save(writer);
			writer.write<int32_t>(bone.geoset_id);
			writer.write<int32_t>(bone.geoset_animation_id);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_TEXS_chunk(BinaryWriter& writer) const {
		if (textures.empty()) {
			return;
		}

		writer.write(ChunkTag::TEXS);
		writer.write<uint32_t>(textures.size() * 268);
		for (const auto& texture : textures) {
			writer.write<uint32_t>(texture.replaceable_id);
			writer.write_c_string_padded(texture.file_name.string(), 260);
			writer.write<uint32_t>(texture.flags);
		}
	}

	void MDX::write_LITE_chunk(BinaryWriter& writer) const {
		if (lights.empty()) {
			return;
		}

		writer.write(ChunkTag::LITE);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& light : lights) {
			// Write temporary zero, remember location
			const size_t light_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			light.node.save(writer);
			writer.write<uint32_t>(light.type);
			writer.write<float>(light.attenuation_start);
			writer.write<float>(light.attenuation_end);
			writer.write<glm::vec3>(light.color);
			writer.write<float>(light.intensity);
			writer.write<glm::vec3>(light.ambient_color);
			writer.write<float>(light.ambient_intensity);

			light.KLAS.save(TrackTag::KLAS, writer);
			light.KLAE.save(TrackTag::KLAE, writer);
			light.KLAC.save(TrackTag::KLAC, writer);
			light.KLAI.save(TrackTag::KLAI, writer);
			light.KLBI.save(TrackTag::KLBI, writer);
			light.KLBC.save(TrackTag::KLBC, writer);
			light.KLAV.save(TrackTag::KLAV, writer);

			const uint32_t temporary = writer.buffer.size() - light_index;
			std::memcpy(writer.buffer.data() + light_index, &temporary, 4);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_HELP_chunk(BinaryWriter& writer) const {
		if (help_bones.empty()) {
			return;
		}

		writer.write(ChunkTag::HELP);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& help_bone : help_bones) {
			help_bone.save(writer);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_ATCH_chunk(BinaryWriter& writer) const {
		if (attachments.empty()) {
			return;
		}

		writer.write(ChunkTag::ATCH);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& attachment : attachments) {
			// Write temporary zero, remember location
			const size_t attachment_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			attachment.node.save(writer);
			writer.write_c_string_padded(attachment.path, 256);
			writer.write<uint32_t>(attachment.reserved);
			writer.write<uint32_t>(attachment.attachment_id);

			attachment.KATV.save(TrackTag::KATV, writer);

			const uint32_t temporary = writer.buffer.size() - attachment_index;
			std::memcpy(writer.buffer.data() + attachment_index, &temporary, 4);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_PIVT_chunk(BinaryWriter& writer) const {
		if (pivots.empty()) {
			return;
		}

		writer.write(ChunkTag::PIVT);
		writer.write<uint32_t>(pivots.size() * 12);
		for (const auto& pivot : pivots) {
			writer.write<glm::vec3>(pivot);
		}
	}

	void MDX::write_PREM_chunk(BinaryWriter& writer) const {
		if (emitters1.empty()) {
			return;
		}

		writer.write(ChunkTag::PREM);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& emitter : emitters1) {
			// Write temporary zero, remember location
			const size_t emitter_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			emitter.node.save(writer);
			writer.write<float>(emitter.emission_rate);
			writer.write<float>(emitter.gravity);
			writer.write<float>(emitter.longitude);
			writer.write<float>(emitter.latitude);
			writer.write_c_string_padded(emitter.path, 260);
			writer.write<uint32_t>(emitter.reserved);
			writer.write<float>(emitter.life_span);
			writer.write<float>(emitter.speed);

			emitter.KPEE.save(TrackTag::KPEE, writer);
			emitter.KPEG.save(TrackTag::KPEG, writer);
			emitter.KPLN.save(TrackTag::KPLN, writer);
			emitter.KPLT.save(TrackTag::KPLT, writer);
			emitter.KPEL.save(TrackTag::KPEL, writer);
			emitter.KPES.save(TrackTag::KPES, writer);
			emitter.KPEV.save(TrackTag::KPEV, writer);

			const uint32_t temporary = writer.buffer.size() - emitter_index;
			std::memcpy(writer.buffer.data() + emitter_index, &temporary, 4);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_PRE2_chunk(BinaryWriter& writer) const {
		if (emitters2.empty()) {
			return;
		}

		writer.write(ChunkTag::PRE2);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& emitter : emitters2) {
			// Write temporary zero, remember location
			const size_t emitter_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			emitter.node.save(writer);
			writer.write<float>(emitter.speed);
			writer.write<float>(emitter.variation);
			writer.write<float>(emitter.latitude);
			writer.write<float>(emitter.gravity);
			writer.write<float>(emitter.life_span);
			writer.write<float>(emitter.emission_rate);
			writer.write<float>(emitter.length);
			writer.write<float>(emitter.width);
			writer.write<uint32_t>(emitter.filter_mode);
			writer.write<uint32_t>(emitter.rows);
			writer.write<uint32_t>(emitter.columns);
			writer.write<uint32_t>(emitter.head_or_tail);
			writer.write<float>(emitter.tail_length);
			writer.write<float>(emitter.time_middle);

			writer.write<glm::vec3>(emitter.start_segment_color);
			writer.write<glm::vec3>(emitter.middle_segment_color);
			writer.write<glm::vec3>(emitter.end_segment_color);

			writer.write<glm::u8vec3>(emitter.segment_alphas);
			writer.write<glm::vec3>(emitter.segment_scaling);
			writer.write<glm::uvec3>(emitter.head_intervals);
			writer.write<glm::uvec3>(emitter.head_decay_intervals);
			writer.write<glm::uvec3>(emitter.tail_intervals);
			writer.write<glm::uvec3>(emitter.tail_decay_intervals);

			writer.write<uint32_t>(emitter.texture_id);
			writer.write<uint32_t>(emitter.squirt);
			writer.write<uint32_t>(emitter.priority_plane);
			writer.write<uint32_t>(emitter.replaceable_id);

			emitter.KP2R.save(TrackTag::KP2R, writer);
			emitter.KP2L.save(TrackTag::KP2L, writer);
			emitter.KP2G.save(TrackTag::KP2G, writer);
			emitter.KP2E.save(TrackTag::KP2E, writer);
			emitter.KP2N.save(TrackTag::KP2N, writer);
			emitter.KP2W.save(TrackTag::KP2W, writer);
			emitter.KP2V.save(TrackTag::KP2V, writer);

			const uint32_t temporary = writer.buffer.size() - emitter_index;
			std::memcpy(writer.buffer.data() + emitter_index, &temporary, 4);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_RIBB_chunk(BinaryWriter& writer) const {
		if (ribbons.empty()) {
			return;
		}

		writer.write(ChunkTag::RIBB);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& ribbon : ribbons) {
			// Write temporary zero, remember location
			const size_t ribbon_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			ribbon.node.save(writer);
			writer.write<float>(ribbon.height_above);
			writer.write<float>(ribbon.height_below);
			writer.write<float>(ribbon.alpha);
			writer.write<glm::vec3>(ribbon.color);
			writer.write<float>(ribbon.life_span);
			writer.write<uint32_t>(ribbon.texture_slot);
			writer.write<uint32_t>(ribbon.emission_rate);
			writer.write<uint32_t>(ribbon.rows);
			writer.write<uint32_t>(ribbon.columns);
			writer.write<uint32_t>(ribbon.material_id);
			writer.write<float>(ribbon.gravity);
			
			ribbon.KRHA.save(TrackTag::KRHA, writer);
			ribbon.KRHB.save(TrackTag::KRHB, writer);
			ribbon.KRAL.save(TrackTag::KRAL, writer);
			ribbon.KRCO.save(TrackTag::KRCO, writer);
			ribbon.KRTX.save(TrackTag::KRTX, writer);
			ribbon.KRVS.save(TrackTag::KRVS, writer);

			const uint32_t temporary = writer.buffer.size() - ribbon_index;
			std::memcpy(writer.buffer.data() + ribbon_index, &temporary, 4);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_EVTS_chunk(BinaryWriter& writer) const {
		if (eventObjects.empty()) {
			return;
		}

		writer.write(ChunkTag::EVTS);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& event_object : eventObjects) {
			event_object.node.save(writer);
			writer.write_string("KEVT");
			writer.write<uint32_t>(event_object.times.size());
			writer.write<int32_t>(event_object.global_sequence_id);
			writer.write_vector(event_object.times);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_CLID_chunk(BinaryWriter& writer) const {
		if (collisionShapes.empty()) {
			return;
		}

		writer.write(ChunkTag::CLID);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& shape : collisionShapes) {
			shape.node.save(writer);
			writer.write<uint32_t>(static_cast<uint32_t>(shape.type));

			if (shape.type == CollisionShapeType::Sphere) {
				writer.write<glm::vec3>(shape.vertices[0]);
			} else {
				writer.write<glm::vec3>(shape.vertices[0]);
				writer.write<glm::vec3>(shape.vertices[1]);
			}
			if (shape.type == CollisionShapeType::Sphere || shape.type == CollisionShapeType::Cylinder) {
				writer.write<float>(shape.radius);
			}
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_CORN_chunk(BinaryWriter& writer) const {
		if (corn_emitters.empty()) {
			return;
		}

		writer.write(ChunkTag::CORN);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& corn : corn_emitters) {
			// Write temporary zero, remember location
			const size_t corn_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			corn.node.save(writer);
			writer.write_vector(corn.data);

			const uint32_t temporary = writer.buffer.size() - corn_index;
			std::memcpy(writer.buffer.data() + corn_index, &temporary, 4);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_CAMS_chunk(BinaryWriter& writer) const {
		if (cameras.empty()) {
			return;
		}

		writer.write(ChunkTag::CAMS);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& camera : cameras) {
			// Write temporary zero, remember location
			const size_t camera_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			writer.write_vector(camera.data);

			const uint32_t temporary = writer.buffer.size() - camera_index;
			std::memcpy(writer.buffer.data() + camera_index, &temporary, 4);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_BPOS_chunk(BinaryWriter& writer) const {
		if (bind_poses.empty()) {
			return;
		}

		writer.write(ChunkTag::BPOS);
		writer.write<uint32_t>(4 + bind_poses.size() * 4);
		writer.write<uint32_t>(bind_poses.size() / 12);
		writer.write_vector(bind_poses);
	}

	void MDX::write_TXAN_chunk(BinaryWriter& writer) const {
		if (texture_animations.empty()) {
			return;
		}

		writer.write(ChunkTag::TXAN);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& texture_animation : texture_animations) {
			// Write temporary zero, remember location
			const size_t texture_animation_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			writer.write_vector(texture_animation.data);

			const uint32_t temporary = writer.buffer.size() - texture_animation_index;
			std::memcpy(writer.buffer.data() + texture_animation_index, &temporary, 4);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void MDX::write_FAFX_chunk(BinaryWriter& writer) const {
		if (facefxes.empty()) {
			return;
		}

		writer.write(ChunkTag::FAFX);
		writer.write<uint32_t>(facefxes.size() * 340);
		for (const auto& facefx : facefxes) {
			writer.write_c_string_padded(facefx.name, 80);
			writer.write_c_string_padded(facefx.path.string(), 260);
		}
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
			fmt::print("Incorrect file magic number. Should be MDLX, is {}\n", magic_number);
			return;
		}

		while (reader.remaining() > 0) {
			uint32_t header = reader.read<uint32_t>();

			switch (static_cast<ChunkTag>(header)) {
				case ChunkTag::VERS:
					reader.advance(4);
					version = reader.read<uint32_t>();
					break;
				case ChunkTag::MODL:
					reader.advance(4);
					name = reader.read_string(80);
					animation_filename = reader.read_string(260);
					extent = Extent(reader);
					blend_time = reader.read<uint32_t>();
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
				case ChunkTag::FAFX:
					read_FAFX_chunk(reader);
					break;
				case ChunkTag::CAMS:
					read_CAMS_chunk(reader);
					break;
				case ChunkTag::BPOS:
					read_BPOS_chunk(reader);
					break;
				case ChunkTag::TXAN:
					read_TXAN_chunk(reader);
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

		writer.write(ChunkTag::MODL);
		writer.write<uint32_t>(372);
		writer.write_c_string_padded(name, 80);
		writer.write_c_string_padded(animation_filename, 260);
		extent.save(writer);
		writer.write<uint32_t>(blend_time);

		write_SEQS_chunk(writer);
		write_MTLS_chunk(writer);
		write_TEXS_chunk(writer);
		write_GEOS_chunk(writer);
		write_GEOA_chunk(writer);
		write_BONE_chunk(writer);
		write_GLBS_chunk(writer);
		write_LITE_chunk(writer);
		write_HELP_chunk(writer);
		write_ATCH_chunk(writer);
		write_PIVT_chunk(writer);
		write_PREM_chunk(writer);
		write_PRE2_chunk(writer);
		write_RIBB_chunk(writer);
		write_CAMS_chunk(writer);
		write_EVTS_chunk(writer);
		write_CLID_chunk(writer);
		write_CORN_chunk(writer);
		write_FAFX_chunk(writer);
		write_BPOS_chunk(writer);
		write_TXAN_chunk(writer);

		std::ofstream file(path, std::ios::binary | std::ios::out);
		file.write(reinterpret_cast<char*>(writer.buffer.data()), writer.buffer.size());
	}

	void MDX::validate() {
		// Remove geoset animations that reference non existing geosets
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
						eventObjects.size() +
						collisionShapes.size() +
						corn_emitters.size();

		// If there are no bones we have to add one to prevent crashing and stuff.
		if (bones.empty()) {
			Bone bone{};
			bone.node.parent_id = -1;
			bone.node.id = node_count++;
			bones.push_back(bone);
		}

		// Ensure that pivots is big enough
		pivots.resize(node_count, {});
		
		// Compact node IDs
		std::vector<int> IDs;
		IDs.reserve(node_count);
		forEachNode([&](mdx::Node& node) {
			if (node.id == -1) {
				fmt::print("Invalid node \"{}\" with ID -1\n", node.name);
				return;
			}
			IDs.push_back(node.id);
		});

		const int max_id = *std::max_element(IDs.begin(), IDs.end());
		std::vector<int> remapping(max_id + 1);
		for (int i = 0; i < IDs.size(); i++) {
			remapping[IDs[i]] = i;
		}

		forEachNode([&](mdx::Node& node) {
			if (node.id == -1) {
				fmt::print("Invalid node \"{}\" with ID -1\n", node.name);
				return;
			}
			node.id = remapping[node.id];
			if (node.parent_id != -1) {
				node.parent_id = remapping[node.parent_id];
			}
		});

		// Fix vertex groups that reference non existent matrix groups
		for (auto& i : geosets) {
			for (auto& j : i.vertex_groups) {
				// If no matrix groups exist we insert one
				if (i.matrix_groups.empty()) {
					i.matrix_groups.push_back(1);
					i.matrix_indices.push_back(0);
				}
				// Don't reference non existing ones!
				if (j >= i.matrix_groups.size()) {
					j = std::min<uint8_t>(j, i.matrix_groups.size() - 1);
				}
			}
		}
	}

	void MDX::optimize() {
		Bone& bone = bones.front();
		auto& header = bone.node.KGTR;
		auto new_tracks = header.tracks;

		Sequence& current_sequence = sequences.front();
		//for (const auto& track : header.tracks) {
		for (int i = 0; i < new_tracks.size(); i++) {
			auto& track = new_tracks[i];

			if (track.frame > current_sequence.end_frame) {
				for (const auto& i : sequences) {
					if (i.start_frame <= track.frame && i.end_frame >= track.frame) {
						current_sequence = i;
						break;
					}
				}
				// If we find a track that lies outside any sequence we skip it
				if (track.frame > current_sequence.end_frame) {
					continue;
				}
			}
		}

		if (header.interpolation_type == 1) {
			for (const auto& i : sequences) {
				
			}
			auto trackA = header.tracks[0];
			auto trackB = header.tracks[1];
			auto trackC = header.tracks[2];

			int32_t diffAB = trackB.frame - trackA.frame;
			int32_t diffBC = trackC.frame - trackB.frame;
			int32_t total = trackC.frame - trackA.frame;

			glm::vec3 between = trackA.value + trackC.value * (static_cast<float>(diffAB) / total);
			glm::vec3 diff = (trackB.value - between) / between * 100.f;
			if (diff.x < 1.f && diff.y < 1.f && diff.z < 1.f) {
				fmt::print("yeet");
			}
		}
	}
} // namespace mdx