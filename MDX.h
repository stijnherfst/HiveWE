#pragma once

namespace mdx {
	extern std::map<int, std::string> replacable_id_to_texture;

	enum class TrackTag {
		KMTF = 0x46544d4b,
		KMTA = 0x41544d4b,
		KTAT = 0x5441544b,
		KTAR = 0x5241544b,
		KTAS = 0x5341544b,
		KGAO = 0x4f41474b,
		KGAC = 0x4341474b,
		KLAS = 0x53414c4b,
		KLAE = 0x45414c4b,
		KLAC = 0x43414c4b,
		KLAI = 0x49414c4b,
		KLBI = 0x49424c4b,
		KLBC = 0x43424c4b,
		KLAV = 0x56414c4b,
		KATV = 0x5654414b,
		KPEE = 0x4545504b,
		KPEG = 0x4745504b,
		KPLN = 0x4e4c504b,
		KPLT = 0x544c504b,
		KPEL = 0x4c45504b,
		KPES = 0x5345504b,
		KPEV = 0x5645504b,
		KP2S = 0x5332504b,
		KP2R = 0x5232504b,
		KP2L = 0x4c32504b,
		KP2G = 0x4732504b,
		KP2E = 0x4532504b,
		KP2N = 0x4e32504b,
		KP2W = 0x5732504b,
		KP2V = 0x5632504b,
		KRHA = 0x4148524b,
		KRHB = 0x4248524b,
		KRAL = 0x4c41524b,
		KRCO = 0x4f43524b,
		KRTX = 0x5854524b,
		KRVS = 0x5356524b,
		KCTR = 0x5254434b,
		KTTR = 0x5254544b,
		KCRL = 0x4c52434b,
		KGTR = 0x5254474b,
		KGRT = 0x5452474b,
		KGSC = 0x4353474b
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

	template <typename T>
	struct Track {
		int32_t frame;
		T value;
		T inTan;
		T outTan;
	};

	struct TrackHeaderBase {
		int32_t interpolation_type;
		int32_t global_sequence_ID;

		virtual ~TrackHeaderBase() = default;
	};

	template <typename T>
	struct TrackHeader : TrackHeaderBase {
		std::vector<Track<T>> tracks;

		explicit TrackHeader(BinaryReader& reader) {
			const int tracks_count = reader.read<int32_t>();
			interpolation_type = reader.read<int32_t>();
			global_sequence_ID = reader.read<int32_t>();

			for (int i = 0; i < tracks_count; i++) {
				Track<T> track;
				track.frame = reader.read<int32_t>();
				track.value = reader.read<T>();
				if (interpolation_type > 1) {
					track.inTan = reader.read<T>();
					track.outTan = reader.read<T>();
				}
				tracks.push_back(track);
			}
		}

		~TrackHeader() = default;
	};

	struct AnimatedData {
		std::unordered_map<TrackTag, std::shared_ptr<TrackHeaderBase>> tracks;

		void load_tracks(BinaryReader& reader);

		AnimatedData() = default;
		AnimatedData(const AnimatedData&) = default;
		AnimatedData(AnimatedData&&) = default;
		AnimatedData& operator=(const AnimatedData&) = default;

		template<typename T>
		std::shared_ptr<TrackHeader<T>> track(const TrackTag track) {
			return std::dynamic_pointer_cast<TrackHeader<T>>(tracks[track]);
		}

		bool has_track(const TrackTag track) {
			return tracks.contains(track);
		}

	};

	struct Extent {
		float bounds_radius;
		glm::vec3 minimum;
		glm::vec3 maximum;

		Extent() = default;
		explicit Extent(BinaryReader& reader);
	};

	struct TextureCoordinateSet {
		TextureCoordinateSet() = default;
		explicit TextureCoordinateSet(BinaryReader& reader);
		std::vector<glm::vec2> coordinates;
	};

	struct Layer {
		Layer() = default;
		explicit Layer(BinaryReader& reader);

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
	};

	struct Node {
		Node() = default;
		explicit Node(BinaryReader& reader);

		std::string name;
		int object_id;
		int parent_id;
		int flags;
		AnimatedData animated_data;
	};

	struct Sequence {
		explicit Sequence(BinaryReader& reader);

		std::string name;
		uint32_t interval_start;
		uint32_t interval_end;
		float movespeed;
		uint32_t flags; // 0: looping
						// 1: non looping
		float rarity;
		uint32_t sync_point;
		Extent extent;
	};

	struct Geoset {
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
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

	struct GeosetAnimation {
		float alpha;
		uint32_t flags;
		glm::vec3 color;
		uint32_t geoset_id;
		AnimatedData animated_data;
	};

	struct Texture {
		explicit Texture(BinaryReader& reader);
		uint32_t replaceable_id;
		std::string file_name;
		uint32_t flags;
	};

	struct Material {
		uint32_t priority_plane;
		uint32_t flags;
		std::vector<Layer> layers;
	};

	struct Bone {
		Node node;
		int geoset_id;
		int geoset_animation_id;
	};

	struct Chunk {
		virtual ~Chunk() = default;
	};

	class VERS : public Chunk {
		uint32_t version;

		static const ChunkTag tag = ChunkTag::VERS;
	};

	struct SEQS : Chunk {
		explicit SEQS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::SEQS;
		std::vector<Sequence> sequences;
	};

	struct GEOS : Chunk {
		explicit GEOS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::GEOS;
		std::vector<Geoset> geosets;
	};

	struct GEOA : Chunk {
		explicit GEOA(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::GEOA;
		std::vector<GeosetAnimation> animations;
	};

	struct TEXS : Chunk {
		explicit TEXS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::TEXS;
		std::vector<Texture> textures;
	};

	struct MTLS : Chunk {
		explicit MTLS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::MTLS;
		std::vector<Material> materials;
	};

	struct BONE : Chunk {
		explicit BONE(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::BONE;
		std::vector<Bone> bones;
	};

	class MDX {
		std::map<ChunkTag, std::shared_ptr<Chunk>> chunks;

	public:
		explicit MDX(BinaryReader& reader);
		void load(BinaryReader& reader);

		template<typename T>
		std::shared_ptr<T> chunk() {
			static_assert(std::is_base_of<Chunk, T>::value, "T must inherit from Chunk");

			return std::dynamic_pointer_cast<T>(chunks[static_cast<ChunkTag>(T::tag)]);
		}

		template<typename T>
		bool has_chunk() {
			static_assert(std::is_base_of<Chunk, T>::value, "T must inherit from Chunk");

			return chunks.contains(static_cast<ChunkTag>(T::tag));
		}
	};
}