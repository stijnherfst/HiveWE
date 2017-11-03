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
		coordinates = reader.readVector<float>(texture_coordinates_count * 2);
	}

	Texture::Texture(BinaryReader& reader) {
		replaceable_id = reader.read<uint32_t>();
		file_name = reader.readString(260);
		flags = reader.read<uint32_t>();
	}

	GEOS::GEOS(BinaryReader& reader) {
		uint32_t size = reader.read<uint32_t>();

		uint32_t total_size = 0;

		while (total_size < size) {
			Geoset geoset;
			geoset.inclusive_size = reader.read<uint32_t>();
			reader.position += 4;
			uint32_t vertex_count = reader.read<uint32_t>();
			geoset.vertices = reader.readVector<float>(vertex_count * 3);
			reader.position += 4;
			uint32_t normal_count = reader.read<uint32_t>();
			geoset.normals = reader.readVector<float>(normal_count * 3);
			reader.position += 4;
			uint32_t face_type_groups_count = reader.read<uint32_t>();
			geoset.face_type_groups = reader.readVector<uint32_t>(face_type_groups_count);
			reader.position += 4;
			uint32_t face_groups_count = reader.read<uint32_t>();
			geoset.face_groups = reader.readVector<uint32_t>(face_groups_count);
			reader.position += 4;
			uint32_t faces_count = reader.read<uint32_t>();
			geoset.faces = reader.readVector<uint16_t>(faces_count);
			reader.position += 4;
			uint32_t vertex_groups_count = reader.read<uint32_t>();
			geoset.vertex_groups = reader.readVector<uint8_t>(vertex_groups_count);
			reader.position += 4;
			uint32_t matrix_group_count = reader.read<uint32_t>();
			geoset.matrix_groups = reader.readVector<uint32_t>(matrix_group_count);
			reader.position += 4;
			uint32_t matrix_indices_count = reader.read<uint32_t>();
			geoset.matrix_indices = reader.readVector<uint32_t>(matrix_indices_count);
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
			total_size += geoset.inclusive_size;
		}
	}

	TEXS::TEXS(BinaryReader& reader) {
		uint32_t size = reader.read<uint32_t>();

		for (size_t i = 0; i < size / 268; i++) {
			textures.push_back(Texture(reader));
		}
	}

	MDX::MDX(std::string path) {
		load(path);
	}

	void MDX::load(const std::string& path) {
		BinaryReader reader(hierarchy.open_file(path));

		std::string magic_number = reader.readString(4);
		if (magic_number != "MDLX") {
			std::cout << "The files magic number is incorrect. Should be MDLX, is: " << magic_number << std::endl;
			return;
		}

		uint32_t header;
		uint32_t size;
		while (reader.remaining() > 0) {
			header = reader.read<uint32_t>();
			size = reader.read<uint32_t>();
			reader.position -= 4;

			//uint32_t headerr = reader.read<uint32_t>();
			//reader.position -= 4;
			//std::string header = reader.readString(4);
			//uint32_t size = reader.read<uint32_t>();
			//std::cout << header << " " << headerr << std::endl;
			//reader.position += size;

			switch (static_cast<ChunkTag>(header)) {
				case ChunkTag::GEOS:
					chunks[ChunkTag::GEOS] = new GEOS(reader);
					break;
				case ChunkTag::TEXS:
					chunks[ChunkTag::TEXS] = new TEXS(reader);
					break;
				//case ChunkTag::MTLS:
				//	break;
				default:
					reader.position += size + 4;
					continue;
			}
		}
	}
}