#pragma once

namespace mdx {
	struct Extent {
		float bounds_radius;
		glm::vec3 minimum;
		glm::vec3 maximum;

		Extent() = default;
		Extent(BinaryReader& reader);
	};

	struct TextureCoordinateSet {
		TextureCoordinateSet() = default;
		TextureCoordinateSet(BinaryReader& reader);
		std::vector<glm::vec2> coordinates;
	};

	struct Layer {
		uint32_t blend_mode; // 0: none
							// 1: transparent
							// 2: blend
							// 3: additive
							// 4: add alpha
							// 5: modulate
							// 6: modulate 2x
		uint32_t shading_flags; // 0x1: unshaded
							// 0x2: sphere environment map
							// 0x4: ?
							// 0x8: ?
							// 0x10: two sided
							// 0x20: unfogged
							// 0x30: no depth test
							// 0x40: no depth set
		uint32_t texture_id;
		uint32_t texture_animation_id;
		uint32_t coord_id;
		float alpha;
		//(KMTF)
		//(KMTA)

		Layer() = default;
		Layer(BinaryReader& reader);
	};

//KMTF: uint32 textureId
//	KMTA : float alpha

	struct Texture {
		Texture(BinaryReader& reader);
		uint32_t replaceable_id;
		std::string file_name;
		uint32_t flags;
	};

	struct Geoset {
		std::vector<glm::vec3> vertices;
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

	struct Material {
		uint32_t priority_plane;
		uint32_t flags;
		std::vector<Layer> layers;
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
		virtual ~Chunk() = default;
	};

	class VERS : public Chunk {
		uint32_t version;

		static const ChunkTag tag = ChunkTag::VERS;
	};

	struct GEOS : public Chunk {
		GEOS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::GEOS;
		std::vector<Geoset> geosets;
	};

	struct TEXS : public Chunk {
		TEXS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::TEXS;
		std::vector<Texture> textures;
	};

	struct MTLS : public Chunk {
		MTLS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::MTLS;
		std::vector<Material> materials;
	};

	class MDX {
		std::map<ChunkTag, std::shared_ptr<Chunk>> chunks;
	
	public:
		MDX(BinaryReader& reader);
		void load(BinaryReader& reader);

		template<typename T>
		std::shared_ptr<T> chunk() {
			static_assert(std::is_base_of<Chunk, T>::value, "T must inherit from Chunk");

			return std::dynamic_pointer_cast<T>(chunks[T::tag]);
		}

		template<typename T>
		bool has_chunk() {
			static_assert(std::is_base_of<Chunk, T>::value, "T must inherit from Chunk");

			return chunks.find(T::tag) != chunks.end();
		}
	};
}