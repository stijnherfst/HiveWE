#pragma once

namespace mdx {
	struct Extent {
		float bounds_radius;
		float minimum[3];
		float maximum[3];

		Extent() {};
		Extent(BinaryReader& reader);
	};

	struct TextureCoordinateSet {
		TextureCoordinateSet() {};
		TextureCoordinateSet(BinaryReader& reader);
		std::vector<float> coordinates;

	};

	struct Texture {
		Texture(BinaryReader& reader);
		uint32_t replaceable_id;
		std::string file_name;
		uint32_t flags;
	};

	struct Geoset {
		uint32_t inclusive_size;
		std::vector<float> vertices;
		std::vector<float> normals;
		std::vector<uint32_t> face_type_groups;
		std::vector<uint32_t> face_groups;
		std::vector<uint16_t> faces;
		std::vector<uint8_t> vertex_groups;
		std::vector<uint32_t> matrix_groups;
		std::vector<uint32_t> matrix_indices;

		uint32_t material_id;
		uint32_t selection_group;
		uint32_t selection_flags;
		Extent extent;

		std::vector<Extent> extents;
		std::vector<TextureCoordinateSet> texture_coordinate_sets;
	};

	enum class ChunkTag {
		VERS = 1397900630,
		MODL = 1279545165,
		SEQS = 1397835091,
		MTLS = 1397511245,
		TEXS = 1398293844,
		GEOS = 1397704007,
		GEOA = 1095714119,
		BONE = 1162760002,
		HELP = 1347175752,
		ATCH = 1212372033,
		PIVT = 1414941008,
		EVTS = 1398036037,
		CLID = 1145654339
	};

	struct Chunk {
		virtual ~Chunk() {};
	};

	class VERS : public Chunk {
		uint32_t version;
		~VERS() {}
	};

	class GEOS : public Chunk {
	public:
		GEOS(BinaryReader& reader);
		~GEOS() {}
		std::vector<Geoset> geosets;
	};

	class TEXS : public Chunk {
	public:
		TEXS(BinaryReader& reader);
		~TEXS() {}
		std::vector<Texture> textures;
	};

	class MTLS : public Chunk {
		~MTLS() {}
		//std::vector<Material> materials;
	};

	class MDX {
	public:
		std::map<ChunkTag, Chunk*> chunks;

		MDX(BinaryReader& reader);
		void load(BinaryReader& reader);
	};
}