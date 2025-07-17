module MDX;

import std;
import BinaryReader;
import <glm/glm.hpp>;

namespace mdx {
	void read_GEOS(BinaryReader& reader, MDX& mdx) {
		const uint32_t size = reader.read<uint32_t>();
		uint32_t total_size = 0;

		while (total_size < size) {
			total_size += reader.read<uint32_t>();

			Geoset geoset;
			reader.advance(4); // VRTX
			const uint32_t vertex_count = reader.read<uint32_t>();
			geoset.vertices = reader.read_vector<glm::vec3>(vertex_count);

			reader.advance(4); // NRMS
			const uint32_t normal_count = reader.read<uint32_t>();
			geoset.normals = reader.read_vector<glm::vec3>(normal_count);

			reader.advance(4); // PTYP
			const uint32_t face_type_groups_count = reader.read<uint32_t>();
			geoset.face_type_groups = reader.read_vector<uint32_t>(face_type_groups_count);

			reader.advance(4); // PCNT
			const uint32_t face_groups_count = reader.read<uint32_t>();
			geoset.face_groups = reader.read_vector<uint32_t>(face_groups_count);

			reader.advance(4); // PVTX
			const uint32_t faces_count = reader.read<uint32_t>();
			geoset.faces = reader.read_vector<uint16_t>(faces_count);

			reader.advance(4); // GNDX
			const uint32_t vertex_groups_count = reader.read<uint32_t>();
			geoset.vertex_groups = reader.read_vector<uint8_t>(vertex_groups_count);

			reader.advance(4); // MTGC
			const uint32_t matrix_group_count = reader.read<uint32_t>();
			geoset.matrix_groups = reader.read_vector<uint32_t>(matrix_group_count);

			reader.advance(4); // MATS
			const uint32_t matrix_indices_count = reader.read<uint32_t>();
			geoset.matrix_indices = reader.read_vector<uint32_t>(matrix_indices_count);

			geoset.material_id = reader.read<uint32_t>();
			geoset.selection_group = reader.read<uint32_t>();
			geoset.selection_flags = reader.read<uint32_t>();

			if (mdx.version > 800) {
				geoset.lod = reader.read<uint32_t>();
				geoset.lod_name = reader.read_string(80); // lod name
			} else {
				geoset.lod = 0;
			}

			geoset.extent = Extent(reader);
			const uint32_t extents_count = reader.read<uint32_t>();
			for (size_t i = 0; i < extents_count; i++) {
				geoset.sequence_extents.emplace_back(Extent(reader));
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
				reader.advance(4); // UVBS
				const uint32_t texture_coordinates_count = reader.read<uint32_t>();
				geoset.uv_sets.push_back(reader.read_vector<glm::vec2>(texture_coordinates_count));
			}

			mdx.geosets.push_back(std::move(geoset));
		}
	}

	// Transform from the old version format to our new v1100 based internal representation
	void read_MTLS_texs_pre_v1100(BinaryReader& reader, bool is_hd, uint32_t version, Material& material, int& unique_tracks) {
		reader.advance(4);
		const uint32_t layers_count = reader.read<uint32_t>();

		// These older versions encoded HD materials by having a layer for each PBR material
		// We combine these into the new format
		if (is_hd) {
			Layer layer;
			layer.hd = is_hd;

			for (size_t i = 0; i < layers_count; i++) {
				const size_t reader_pos = reader.position;
				const uint32_t size = reader.read<uint32_t>();

				// Only the first layer's properties matter
				if (i > 0) {
					reader.advance(8);
					LayerTexture layer_texture;
					layer_texture.id = reader.read<uint32_t>();
					layer.texturess.push_back(layer_texture);

					reader.advance(size - 16);
					continue;
				}

				layer.blend_mode = reader.read<uint32_t>();
				layer.shading_flags = reader.read<uint32_t>();
				LayerTexture layer_texture;
				layer_texture.id = reader.read<uint32_t>();
				layer.texture_animation_id = reader.read<uint32_t>();
				layer.coord_id = reader.read<uint32_t>();
				layer.alpha = reader.read<float>();

				layer.emissive_gain = reader.read<float>();
				layer.fresnel_color = reader.read<glm::vec3>();
				layer.fresnel_opacity = reader.read<float>();
				layer.fresnel_team_color = reader.read<float>();

				while (reader.position < reader_pos + size) {
					TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());
					if (tag == TrackTag::KMTF) {
						layer_texture.KMTF = TrackHeader<uint32_t>(reader, unique_tracks++);
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
						std::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
					}
				}

				layer.texturess.push_back(layer_texture);
			}

			material.layers.push_back(std::move(layer));
		} else {
			for (size_t i = 0; i < layers_count; i++) {
				const size_t reader_pos = reader.position;
				const uint32_t size = reader.read<uint32_t>();
				Layer layer;
				layer.hd = is_hd;
				layer.blend_mode = reader.read<uint32_t>();
				layer.shading_flags = reader.read<uint32_t>();

				LayerTexture layer_texture;
				layer_texture.id = reader.read<uint32_t>();
				layer.texturess.push_back(layer_texture);

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
						layer_texture.KMTF = TrackHeader<uint32_t>(reader, unique_tracks++);
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
						std::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
					}
				}

				material.layers.push_back(std::move(layer));
			}
		}
	}

	void read_MTLS_texs_post_v1100(BinaryReader& reader, Material& material, int& unique_tracks) {
		reader.advance(4);
		const uint32_t layers_count = reader.read<uint32_t>();
		for (size_t i = 0; i < layers_count; i++) {
			const size_t reader_pos = reader.position;
			Layer layer;
			const uint32_t size = reader.read<uint32_t>();
			layer.blend_mode = reader.read<uint32_t>();
			layer.shading_flags = reader.read<uint32_t>();
			reader.advance(4); // skip texture_id
			layer.texture_animation_id = reader.read<uint32_t>();
			layer.coord_id = reader.read<uint32_t>();
			layer.alpha = reader.read<float>();

			layer.emissive_gain = reader.read<float>();
			layer.fresnel_color = reader.read<glm::vec3>();
			layer.fresnel_opacity = reader.read<float>();
			layer.fresnel_team_color = reader.read<float>();

			layer.hd = reader.read<uint32_t>();
			uint32_t texs = reader.read<uint32_t>();
			for (size_t j = 0; j < texs; j++) {
				LayerTexture layer_texture;
				layer_texture.id = reader.read<uint32_t>();
				uint32_t slot = reader.read<uint32_t>(); // always a garbage value?

				TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());
				if (tag == TrackTag::KMTF) {
					layer_texture.KMTF = TrackHeader<uint32_t>(reader, unique_tracks++);
				} else {
					reader.advance(-4);
				}
				layer.texturess.push_back(layer_texture);
			}

			while (reader.position < reader_pos + size) {
				TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());
				if (tag == TrackTag::KMTA) {
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
					std::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
				}
			}

			material.layers.push_back(std::move(layer));
		}
	}

	void read_MTLS(BinaryReader& reader, MDX& mdx) {
		const uint32_t size = reader.read<uint32_t>();
		uint32_t total_size = 0;

		while (total_size < size) {
			total_size += reader.read<uint32_t>();

			Material material;
			material.priority_plane = reader.read<uint32_t>();
			material.flags = reader.read<uint32_t>();

			if (mdx.version < 1100) {
				bool is_hd = false;
				if (mdx.version == 900 || mdx.version == 1000) {
					is_hd = !reader.read_string(80).empty();
				}
				read_MTLS_texs_pre_v1100(reader, is_hd, mdx.version, material, mdx.unique_tracks);
			} else {
				read_MTLS_texs_post_v1100(reader, material, mdx.unique_tracks);
			}

			mdx.materials.push_back(std::move(material));
		}
	}

	void read_SEQS(BinaryReader& reader, MDX& mdx) {
		const uint32_t size = reader.read<uint32_t>();
		for (size_t i = 0; i < size / 132; i++) {
			mdx.sequences.push_back(Sequence {
				.name = reader.read_string(80),
				.start_frame = reader.read<uint32_t>(),
				.end_frame = reader.read<uint32_t>(),
				.movespeed = reader.read<float>(),
				.flags = reader.read<uint32_t>(),
				.rarity = reader.read<float>(),
				.sync_point = reader.read<uint32_t>(),
				.extent = Extent(reader),
			});
		}
	}

	void read_GEOA(BinaryReader& reader, MDX& mdx) {
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
					animation.KGAO = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KGAC) {
					animation.KGAC = TrackHeader<glm::vec3>(reader, mdx.unique_tracks++);
				} else {
					std::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
				}
			}

			mdx.animations.push_back(std::move(animation));
		}
	}

	void read_BONE(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			mdx.bones.push_back(Bone { 
				.node = Node(reader, mdx.unique_tracks),
				.geoset_id = reader.read<int32_t>(),
				.geoset_animation_id = reader.read<int32_t>(),
			});
		}
	}

	void read_TEXS(BinaryReader& reader, MDX& mdx) {
		const uint32_t size = reader.read<uint32_t>();
		for (size_t i = 0; i < size / 268; i++) {
			mdx.textures.push_back(Texture{
				.replaceable_id = reader.read<uint32_t>(),
				.file_name = reader.read_string(260),
				.flags = reader.read<uint32_t>(),
			});
		}
	}

	void read_GLBS(BinaryReader& reader, MDX& mdx) {
		const uint32_t size = reader.read<uint32_t>();
		mdx.global_sequences = reader.read_vector<uint32_t>(size / 4);
	}

	void read_LITE(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Light light;
			const size_t node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			light.node = Node(reader, mdx.unique_tracks);
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
					light.KLAS = TrackHeader<uint32_t>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KLAE) {
					light.KLAE = TrackHeader<uint32_t>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KLAC) {
					light.KLAC = TrackHeader<glm::vec3>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KLAI) {
					light.KLAI = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KLBI) {
					light.KLBI = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KLBC) {
					light.KLBC = TrackHeader<glm::vec3>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KLAV) {
					light.KLAV = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else {
					std::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
				}
			}
			mdx.lights.push_back(std::move(light));
		}
	}

	void read_HELP(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();
		while (reader.position < reader_pos + size) {
			mdx.help_bones.push_back(Node(reader, mdx.unique_tracks));
		}
	}

	void read_ATCH(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Attachment attachment;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			attachment.node = Node(reader, mdx.unique_tracks);
			attachment.path = reader.read_string(256);
			attachment.reserved = reader.read<uint32_t>();
			attachment.attachment_id = reader.read<uint32_t>();
			while (reader.position < node_reader_pos + inclusive_size) {
				TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());
				attachment.KATV = TrackHeader<float>(reader, mdx.unique_tracks++);
				if (tag != TrackTag::KATV) {
					std::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
				}
			}
			mdx.attachments.push_back(std::move(attachment));
		}
	}

	void read_PIVT(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		mdx.pivots = reader.read_vector<glm::vec3>(size / 12);
	}

	void read_PREM(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			ParticleEmitter1 emitter;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter.node = Node(reader, mdx.unique_tracks);
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
					emitter.KPEE = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KPEG) {
					emitter.KPEG = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KPLN) {
					emitter.KPLN = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KPLT) {
					emitter.KPLT = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KPEL) {
					emitter.KPEL = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KPES) {
					emitter.KPES = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KPEV) {
					emitter.KPEV = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else {
					std::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
				}
			}
			mdx.emitters1.push_back(std::move(emitter));
		}
	}

	void read_PRE2(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			ParticleEmitter2 emitter2;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter2.node = Node(reader, mdx.unique_tracks);

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
					emitter2.KP2S = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KP2R) {
					emitter2.KP2R = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KP2L) {
					emitter2.KP2L = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KP2G) {
					emitter2.KP2G = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KP2E) {
					emitter2.KP2E = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KP2N) {
					emitter2.KP2N = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KP2W) {
					emitter2.KP2W = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KP2V) {
					emitter2.KP2V = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else {
					std::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
				}
			}
			mdx.emitters2.push_back(std::move(emitter2));
		}
	}

	void read_RIBB(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			RibbonEmitter emitter;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter.node = Node(reader, mdx.unique_tracks);
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
					emitter.KRHA = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KRHB) {
					emitter.KRHB = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KRAL) {
					emitter.KRAL = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KRCO) {
					emitter.KRCO = TrackHeader<glm::vec3>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KRTX) {
					emitter.KRTX = TrackHeader<uint32_t>(reader, mdx.unique_tracks++);
				} else if (tag == TrackTag::KRVS) {
					emitter.KRVS = TrackHeader<float>(reader, mdx.unique_tracks++);
				} else {
					std::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
				}
			}
			mdx.ribbons.push_back(std::move(emitter));
		}
	}

	void read_EVTS(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			EventObject evt;
			evt.node = Node(reader, mdx.unique_tracks);
			reader.advance(4); // read KEVT
			uint32_t count = reader.read<uint32_t>();
			evt.global_sequence_id = reader.read<int32_t>(); // signed
			evt.times = reader.read_vector<uint32_t>(count);
			mdx.event_objects.push_back(std::move(evt));
		}
	}

	void read_CLID(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			CollisionShape shape;
			shape.node = Node(reader, mdx.unique_tracks);
			shape.type = static_cast<CollisionShape::Shape>(reader.read<uint32_t>());

			for (int i = 0; i < 3; i++) {
				if (reader.remaining() <= 0) {
					shape.vertices[0][i] = 0.f;
				} else {
					shape.vertices[0][i] = reader.read<float>();
				}
			}

			if (shape.type != CollisionShape::Shape::Sphere) {
				for (int i = 0; i < 3; i++) {
					if (reader.remaining() <= 0) {
						shape.vertices[1][i] = 0.f;
					} else {
						shape.vertices[1][i] = reader.read<float>();
					}
				}
			}

			if (shape.type == CollisionShape::Shape::Sphere || shape.type == CollisionShape::Shape::Cylinder) {
				if (reader.remaining() > 0) {
					shape.radius = reader.read<float>();
				} else {
					shape.radius = 0.f;
				}
			}
			mdx.collision_shapes.push_back(std::move(shape));
		}
	}

	void read_CORN(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			CornEmitter emitter;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter.node = Node(reader, mdx.unique_tracks);
			emitter.data = reader.read_vector<uint8_t>(inclusive_size - (reader.position - node_reader_pos));
			mdx.corn_emitters.push_back(std::move(emitter));
		}
	}

	void read_CAMS(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Camera camera;
			const uint32_t inclusive_size = reader.read<uint32_t>();

			camera.data = reader.read_vector<uint8_t>(inclusive_size - 4);
			mdx.cameras.push_back(std::move(camera));
		}
	}

	void read_BPOS(BinaryReader& reader, MDX& mdx) {
		const uint32_t size = reader.read<uint32_t>();
		mdx.bind_poses = reader.read_vector<float>(reader.read<uint32_t>() * 12);
	}

	void read_TXAN(BinaryReader& reader, MDX& mdx) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			TextureAnimation animation;
			const uint32_t inclusive_size = reader.read<uint32_t>();

			animation.data = reader.read_vector<uint8_t>(inclusive_size - 4);
			mdx.texture_animations.push_back(std::move(animation));
		}
	}

	void read_FAFX(BinaryReader& reader, MDX& mdx) {
		const uint32_t size = reader.read<uint32_t>();
		for (size_t i = 0; i < size / 340; i++) {
			FaceFX facefx;
			facefx.name = reader.read_string(80);
			facefx.path = reader.read_string(260);
			mdx.facefxes.push_back(std::move(facefx));
		}
	}

	void MDX::load(BinaryReader& reader) {
		const std::string magic_number = reader.read_string(4);
		if (magic_number != "MDLX") {
			std::print("Incorrect file magic number, expected MDLX but got {}\n", magic_number);
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
					read_GEOS(reader, *this);
					break;
				case ChunkTag::MTLS:
					read_MTLS(reader, *this);
					break;
				case ChunkTag::SEQS:
					read_SEQS(reader, *this);
					break;
				case ChunkTag::GLBS:
					read_GLBS(reader, *this);
					break;
				case ChunkTag::GEOA:
					read_GEOA(reader, *this);
					break;
				case ChunkTag::BONE:
					read_BONE(reader, *this);
					break;
				case ChunkTag::TEXS:
					read_TEXS(reader, *this);
					break;
				case ChunkTag::LITE:
					read_LITE(reader, *this);
					break;
				case ChunkTag::HELP:
					read_HELP(reader, *this);
					break;
				case ChunkTag::ATCH:
					read_ATCH(reader, *this);
					break;
				case ChunkTag::PIVT:
					read_PIVT(reader, *this);
					break;
				case ChunkTag::PREM:
					read_PREM(reader, *this);
					break;
				case ChunkTag::PRE2:
					read_PRE2(reader, *this);
					break;
				case ChunkTag::RIBB:
					read_RIBB(reader, *this);
					break;
				case ChunkTag::EVTS:
					read_EVTS(reader, *this);
					break;
				case ChunkTag::CLID:
					read_CLID(reader, *this);
					break;
				case ChunkTag::CORN:
					read_CORN(reader, *this);
					break;
				case ChunkTag::FAFX:
					read_FAFX(reader, *this);
					break;
				case ChunkTag::CAMS:
					read_CAMS(reader, *this);
					break;
				case ChunkTag::BPOS:
					read_BPOS(reader, *this);
					break;
				case ChunkTag::TXAN:
					read_TXAN(reader, *this);
					break;
				default:
					reader.advance(reader.read<uint32_t>());
			}
		}

		validate();
	}
}