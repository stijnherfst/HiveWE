#include "stdafx.h"

namespace mdx {
	std::map<int, std::string> replacable_id_to_texture{
		{ 1, "ReplaceableTextures/TeamColor/TeamColor00.blp" },
		{ 2, "ReplaceableTextures/TeamGlow/TeamGlow00.blp" },
		{ 11, "ReplaceableTextures/Cliff/Cliff0.blp" },
		{ 31, "ReplaceableTextures/LordaeronTree/LordaeronFallTree.blp" },
		{ 32, "ReplaceableTextures/AshenvaleTree/AshenTree.blp" },
		{ 33, "ReplaceableTextures/BarrensTree/BarrensTree.blp" },
		{ 34, "ReplaceableTextures/NorthrendTree/NorthTree.blp" },
		{ 35, "ReplaceableTextures/Mushroom/MushroomTree.blp" },
		{ 36, "ReplaceableTextures/RuinsTree/RuinsTree.blp" },
		{ 37, "ReplaceableTextures/OutlandMushroomTree/MushroomTree.blp" }
	};

	void AnimatedData::load_tracks(BinaryReader& reader) {
		TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());

		switch (tag) {
			case TrackTag::KMTF:
			case TrackTag::KLAS:
			case TrackTag::KLAE:
			case TrackTag::KRTX:
			case TrackTag::KCRL:
				tracks.emplace(tag, std::make_unique<TrackHeader<uint32_t>>(reader));
				break;
			case TrackTag::KMTA:
			case TrackTag::KGAO:
			case TrackTag::KLAI:
			case TrackTag::KLBI:
			case TrackTag::KLAV:
			case TrackTag::KATV:
			case TrackTag::KPEE:
			case TrackTag::KPEG:
			case TrackTag::KPLN:
			case TrackTag::KPLT:
			case TrackTag::KPEL:
			case TrackTag::KPES:
			case TrackTag::KPEV:
			case TrackTag::KP2S:
			case TrackTag::KP2R:
			case TrackTag::KP2L:
			case TrackTag::KP2G:
			case TrackTag::KP2E:
			case TrackTag::KP2N:
			case TrackTag::KP2W:
			case TrackTag::KP2V:
			case TrackTag::KRHA:
			case TrackTag::KRHB:
			case TrackTag::KRAL:
			case TrackTag::KRVS:
				tracks.emplace(tag, std::make_unique<TrackHeader<float>>(reader));
				break;
			case TrackTag::KTAT:
			case TrackTag::KTAS:
			case TrackTag::KGAC:
			case TrackTag::KLAC:
			case TrackTag::KLBC:
			case TrackTag::KTTR:
			case TrackTag::KCTR:
			case TrackTag::KRCO:
			case TrackTag::KGTR:
			case TrackTag::KGSC:
				tracks.emplace(tag, std::make_unique<TrackHeader<glm::vec3>>(reader));
				break;
			case TrackTag::KTAR:
			case TrackTag::KGRT:
				tracks.emplace(tag, std::make_unique<TrackHeader<glm::vec4>>(reader));
				break;
			default:
				std::cout << "Invalid Track Tag " << static_cast<int>(tag) << "\n";
		}
	}

	Extent::Extent(BinaryReader& reader) {
		bounds_radius = reader.read<float>();
		minimum = reader.read<glm::vec3>();
		maximum = reader.read<glm::vec3>();
	}

	TextureCoordinateSet::TextureCoordinateSet(BinaryReader& reader) {
		reader.advance(4);
		const uint32_t texture_coordinates_count = reader.read<uint32_t>();
		coordinates = reader.read_vector<glm::vec2>(texture_coordinates_count);
	}

	Layer::Layer(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		blend_mode = reader.read<uint32_t>();
		shading_flags = reader.read<uint32_t>();
		texture_id = reader.read<uint32_t>();
		texture_animation_id = reader.read<uint32_t>();
		coord_id = reader.read<uint32_t>();
		alpha = reader.read<float>();

		// Skip tags
		reader.advance(size - 28);
	}

	Texture::Texture(BinaryReader& reader) {
		replaceable_id = reader.read<uint32_t>();
		file_name = reader.read_string(260);
		flags = reader.read<uint32_t>();
	}

	Node::Node(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t inclusive_size = reader.read<uint32_t>();
		name = reader.read_string(80);
		object_id = reader.read<uint32_t>();
		parent_id = reader.read<uint32_t>();
		flags = reader.read<uint32_t>();

		while (reader.position < reader_pos + inclusive_size) {
			animated_data.load_tracks(reader);
		}
	}

	Sequence::Sequence(BinaryReader& reader) {
		name = reader.read_string(80);
		interval_start = reader.read<uint32_t>();
		interval_end = reader.read<uint32_t>();
		movespeed = reader.read<float>();
		flags = reader.read<uint32_t>();
		rarity = reader.read<float>();
		sync_point = reader.read<uint32_t>();
		extent = Extent(reader);
	}

	SEQS::SEQS(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		for (size_t i = 0; i < size / 132; i++) {
			sequences.emplace_back(Sequence(reader));
		}
	}

	GEOS::GEOS(BinaryReader& reader) {
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
			reader.advance(4);
			const uint32_t matrix_indices_count = reader.read<uint32_t>();
			geoset.matrix_indices = reader.read_vector<uint32_t>(matrix_indices_count);
			geoset.material_id = reader.read<uint32_t>();
			geoset.selection_group = reader.read<uint32_t>();
			geoset.selection_flags = reader.read<uint32_t>();

			geoset.extent = Extent(reader);
			const uint32_t extents_count = reader.read<uint32_t>();
			for (size_t i = 0; i < extents_count; i++) {
				geoset.extents.emplace_back(Extent(reader));
			}
			reader.advance(4);
			const uint32_t texture_coordinate_sets_count = reader.read<uint32_t>();
			for (size_t i = 0; i < texture_coordinate_sets_count; i++) {
				geoset.texture_coordinate_sets.emplace_back(TextureCoordinateSet(reader));
			}

			geosets.push_back(geoset);
		}
	}

	GEOA::GEOA(BinaryReader& reader) {
		uint32_t remaining_size = reader.read<uint32_t>();

		while (remaining_size > 0) {
			const int reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			remaining_size -= inclusive_size;

			GeosetAnimation animation;
			animation.alpha = reader.read<float>();
			animation.flags = reader.read<uint32_t>();
			animation.color = reader.read<glm::vec3>();
			animation.geoset_id = reader.read<uint32_t>();

			while (reader.position < reader_pos + inclusive_size) {
				animation.animated_data.load_tracks(reader);
			}

			animations.push_back(std::move(animation));
		}
	}

	TEXS::TEXS(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();

		for (size_t i = 0; i < size / 268; i++) {
			textures.emplace_back(Texture(reader));
		}
	}

	MTLS::MTLS(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		uint32_t total_size = 0;

		while (total_size < size) {
			total_size += reader.read<uint32_t>();
			
			Material material;
			material.priority_plane = reader.read<uint32_t>();
			material.flags = reader.read<uint32_t>();
			reader.advance(4);
			const uint32_t layers_count = reader.read<uint32_t>();

			for (size_t i = 0; i < layers_count; i++) {
				material.layers.emplace_back(Layer(reader));
			}

			materials.push_back(material);
		}
	}

	BONE::BONE(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Bone bone;
			bone.node = Node(reader);
			bone.geoset_id = reader.read<uint32_t>();
			bone.geoset_animation_id = reader.read<uint32_t>();
			bones.push_back(std::move(bone));
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
				case ChunkTag::SEQS:
					chunks[ChunkTag::SEQS] = std::make_shared<SEQS>(reader);
					break;
				case ChunkTag::MTLS:
					chunks[ChunkTag::MTLS] = std::make_shared<MTLS>(reader);
					break;
				case ChunkTag::TEXS:
					chunks[ChunkTag::TEXS] = std::make_shared<TEXS>(reader);
					break;
				case ChunkTag::GEOS:
					chunks[ChunkTag::GEOS] = std::make_shared<GEOS>(reader);
					break;
				case ChunkTag::GEOA:
					chunks[ChunkTag::GEOA] = std::make_shared<GEOA>(reader);
					break;
				case ChunkTag::BONE:
					chunks[ChunkTag::BONE] = std::make_shared<BONE>(reader);
					break;
				default:
					reader.advance(reader.read<uint32_t>());
			}
		}
	}
}