#include "stdafx.h"



namespace mdx {
	Extent::Extent(BinaryReader& reader) {
		bounds_radius = reader.read<float>();
		minimum[0] = reader.read<float>();
		minimum[1] = reader.read<float>();
		minimum[2] = reader.read<float>();
		maximum[0] = reader.read<float>();
		maximum[1] = reader.read<float>();
		maximum[2] = reader.read<float>();
	}

	TextureCoordinateSet::TextureCoordinateSet(BinaryReader& reader) {
		reader.position += 4;
		uint32_t texture_coordinates_count = reader.read<uint32_t>();
		coordinates = reader.read_vector<glm::vec2>(texture_coordinates_count);
	}

	Layer::Layer(BinaryReader& reader) {
		uint32_t size = reader.read<uint32_t>();
		blend_mode = reader.read<uint32_t>();
		shading_flags = reader.read<uint32_t>();
		texture_id = reader.read<uint32_t>();
		texture_animation_id = reader.read<uint32_t>();
		coord_id = reader.read<uint32_t>();
		alpha = reader.read<float>();

		reader.position += size - 28;
	}

	Texture::Texture(BinaryReader& reader) {
		replaceable_id = reader.read<uint32_t>();
		file_name = reader.read_string(260);
		flags = reader.read<uint32_t>();
	}

	GEOS::GEOS(BinaryReader& reader) {
		uint32_t size = reader.read<uint32_t>();
		uint32_t total_size = 0;

		while (total_size < size) {
			total_size += reader.read<uint32_t>();

			Geoset geoset;
			reader.position += 4;
			uint32_t vertex_count = reader.read<uint32_t>();
			geoset.vertices = reader.read_vector<glm::vec3>(vertex_count);
			reader.position += 4;
			uint32_t normal_count = reader.read<uint32_t>();
			geoset.normals = reader.read_vector<float>(normal_count * 3);
			reader.position += 4;
			uint32_t face_type_groups_count = reader.read<uint32_t>();
			geoset.face_type_groups = reader.read_vector<uint32_t>(face_type_groups_count);
			reader.position += 4;
			uint32_t face_groups_count = reader.read<uint32_t>();
			geoset.face_groups = reader.read_vector<uint32_t>(face_groups_count);
			reader.position += 4;
			uint32_t faces_count = reader.read<uint32_t>();
			geoset.faces = reader.read_vector<uint16_t>(faces_count);
			reader.position += 4;
			uint32_t vertex_groups_count = reader.read<uint32_t>();
			geoset.vertex_groups = reader.read_vector<uint8_t>(vertex_groups_count);
			reader.position += 4;
			uint32_t matrix_group_count = reader.read<uint32_t>();
			geoset.matrix_groups = reader.read_vector<uint32_t>(matrix_group_count);
			reader.position += 4;
			uint32_t matrix_indices_count = reader.read<uint32_t>();
			geoset.matrix_indices = reader.read_vector<uint32_t>(matrix_indices_count);
			geoset.material_id = reader.read<uint32_t>();
			geoset.selection_group = reader.read<uint32_t>();
			geoset.selection_flags = reader.read<uint32_t>();

			geoset.extent = Extent(reader);
			uint32_t extents_count = reader.read<uint32_t>();
			for (size_t i = 0; i < extents_count; i++) {
				geoset.extents.push_back(Extent(reader));
			}
			reader.position += 4;
			uint32_t texture_coordinate_sets_count = reader.read<uint32_t>();
			for (size_t i = 0; i < texture_coordinate_sets_count; i++) {
				geoset.texture_coordinate_sets.push_back(TextureCoordinateSet(reader));
			}

			geosets.push_back(geoset);
		}
	}

	TEXS::TEXS(BinaryReader& reader) {
		uint32_t size = reader.read<uint32_t>();

		for (size_t i = 0; i < size / 268; i++) {
			textures.push_back(Texture(reader));
		}
	}

	MTLS::MTLS(BinaryReader& reader) {
		uint32_t size = reader.read<uint32_t>();
		uint32_t total_size = 0;

		while (total_size < size) {
			total_size += reader.read<uint32_t>();
			
			Material material;
			material.priority_plane = reader.read<uint32_t>();
			material.flags = reader.read<uint32_t>();
			reader.position += 4;
			uint32_t layers_count = reader.read<uint32_t>();

			for (size_t i = 0; i < layers_count; i++) {
				material.layers.push_back(Layer(reader));
			}

			materials.push_back(material);
		}
	}

	MDX::MDX(BinaryReader& reader) {
		load(reader);
	}

	void MDX::load(BinaryReader& reader) {
		std::string magic_number = reader.read_string(4);
		if (magic_number != "MDLX") {
			std::cout << "The files magic number is incorrect. Should be MDLX, is: " << magic_number << std::endl;
			return;
		}

		while (reader.remaining() > 0) {
			uint32_t header = reader.read<uint32_t>();

			switch (static_cast<ChunkTag>(header)) {
				case ChunkTag::GEOS:
					chunks[ChunkTag::GEOS] = std::make_shared<GEOS>(reader);
					break;
				case ChunkTag::TEXS:
					chunks[ChunkTag::TEXS] = std::make_shared<TEXS>(reader);
					break;
				case ChunkTag::MTLS:
					chunks[ChunkTag::MTLS] = std::make_shared<MTLS>(reader);
					break;
				default:
					reader.position += reader.read<uint32_t>();
					continue;
			}
		}
	}
}