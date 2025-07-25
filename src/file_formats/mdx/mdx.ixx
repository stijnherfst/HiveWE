export module MDX;

import std;
import BinaryReader;
import BinaryWriter;
import Timer;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;
import <outcome/outcome.hpp>;

namespace fs = std::filesystem;
using OUTCOME_V2_NAMESPACE::failure;
using OUTCOME_V2_NAMESPACE::result;

namespace mdx {
	export extern const std::unordered_map<int, std::string> replaceable_id_to_texture{
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

	enum class TrackTag {
		KMTF = 'FTMK',
		KMTA = 'ATMK',
		KTAT = 'TATK',
		KTAR = 'RATK',
		KTAS = 'SATK',
		KGAO = 'OAGK',
		KGAC = 'CAGK',
		KLAS = 'SALK',
		KLAE = 'EALK',
		KLAC = 'CALK',
		KLAI = 'IALK',
		KLBI = 'IBLK',
		KLBC = 'CBLK',
		KLAV = 'VALK',
		KATV = 'VTAK',
		KPEE = 'EEPK',
		KPEG = 'GEPK',
		KPLN = 'NLPK',
		KPLT = 'TLPK',
		KPEL = 'LEPK',
		KPES = 'SEPK',
		KPEV = 'VEPK',
		KP2S = 'S2PK',
		KP2R = 'R2PK',
		KP2L = 'L2PK',
		KP2G = 'G2PK',
		KP2E = 'E2PK',
		KP2N = 'N2PK',
		KP2W = 'W2PK',
		KP2V = 'V2PK',
		KRHA = 'AHRK',
		KRHB = 'BHRK',
		KRAL = 'LARK',
		KRCO = 'OCRK',
		KRTX = 'XTRK',
		KRVS = 'SVRK',
		KCTR = 'RTCK',
		KTTR = 'RTTK',
		KCRL = 'LRCK',
		KGTR = 'RTGK',
		KGRT = 'TRGK',
		KGSC = 'CSGK',
		KFC3 = '3CFK',
		KFCA = 'ACFK',
		KFTC = 'CTFK',
		KMTE = 'ETMK'
	};

	enum class ChunkTag {
		VERS = 'SREV',
		GEOS = 'SOEG',
		MTLS = 'SLTM',
		SEQS = 'SQES',
		GLBS = 'SBLG',
		GEOA = 'AOEG',
		BONE = 'ENOB',
		TEXS = 'SXET',
		LITE = 'ETIL',
		HELP = 'PLEH',
		ATCH = 'HCTA',
		PIVT = 'TVIP',
		PREM = 'MERP',
		PRE2 = '2ERP',
		RIBB = 'BBIR',
		EVTS = 'STVE',
		CLID = 'DILC',
		CORN = 'NROC',
		SNDS = 'SDNS',
		TXAN = 'NAXT',
		BPOS = 'SOPB',
		FAFX = 'XFAF',
		MODL = 'LDOM',
		CAMS = 'SMAC'
	};

	export template <typename T>
	struct Track {
		int32_t frame;
		T value;
		T inTan;
		T outTan;

		bool operator==(const Track&) const = default;
	};

	export enum class InterpolationType {
		none = 0,
		linear = 1,
		hermite = 2,
		bezier = 3,
	};

	export template <typename T>
	struct TrackHeader {
		InterpolationType interpolation_type = InterpolationType::none;
		int32_t global_sequence_ID = -1;
		std::vector<Track<T>> tracks;

		int id = -1; // Used to track each individual track for animation purposes

		TrackHeader() = default;
		explicit TrackHeader(BinaryReader& reader, int track_id) {
			const uint32_t tracks_count = reader.read<uint32_t>();
			interpolation_type = static_cast<InterpolationType>(reader.read<int32_t>());
			global_sequence_ID = reader.read<int32_t>();
			id = track_id;

			tracks.reserve(tracks_count);
			for (size_t i = 0; i < tracks_count; i++) {
				Track<T> track;
				track.frame = reader.read<int32_t>();
				track.value = reader.read<T>();
				if (interpolation_type == InterpolationType::bezier || interpolation_type == InterpolationType::hermite) {
					track.inTan = reader.read<T>();
					track.outTan = reader.read<T>();
				}
				tracks.push_back(track);
			}
		}

		void save(TrackTag tag, BinaryWriter& writer) const {
			if (tracks.empty()) {
				return;
			}

			writer.write<uint32_t>(static_cast<uint32_t>(tag));
			writer.write<uint32_t>(tracks.size());
			writer.write<uint32_t>(interpolation_type);
			writer.write<uint32_t>(global_sequence_ID);

			for (const auto& i : tracks) {
				writer.write<uint32_t>(i.frame);
				writer.write<T>(i.value);
				if (interpolation_type == InterpolationType::bezier || interpolation_type == InterpolationType::hermite) {
					writer.write<T>(i.inTan);
					writer.write<T>(i.outTan);
				}
			}
		}

		bool operator==(const TrackHeader&) const = default;
	};

	export struct LayerTexture {
		uint32_t id;
		TrackHeader<uint32_t> KMTF;

		bool operator==(const LayerTexture&) const = default;
	};

	export struct Layer {
		uint32_t blend_mode;
		uint32_t shading_flags;
		uint32_t texture_animation_id;
		uint32_t coord_id;
		float alpha;

		float emissive_gain;
		glm::vec3 fresnel_color;
		float fresnel_opacity;
		float fresnel_team_color;

		bool hd;

		std::vector<LayerTexture> texturess;

		TrackHeader<float> KMTA;
		TrackHeader<float> KMTE;
		TrackHeader<glm::vec3> KFC3;
		TrackHeader<float> KFCA;
		TrackHeader<float> KFTC;

		enum ShadingFlags {
			unshaded = 1,
			sphere_environment_map = 2,
			unknown1 = 4,
			unknown2 = 8,
			two_sided = 16,
			unfogged = 32,
			no_depth_test = 64,
			no_depth_set = 128
		};

		bool operator==(const Layer&) const = default;
	};

	export struct Node {
		Node() = default;
		explicit Node(BinaryReader& reader, int& unique_tracks) {
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
					std::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
				}
			}
		}

		void save(BinaryWriter& writer) const {
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

			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		std::string name;
		int id;
		int parent_id;
		int flags;

		TrackHeader<glm::vec3> KGTR;
		TrackHeader<glm::quat> KGRT;
		TrackHeader<glm::vec3> KGSC;

		enum Flags {
			dont_inherit_translation = 0x1,
			dont_inherit_rotation = 0x2,
			dont_inherit_scaling = 0x4,
			billboarded = 0x8,
			billboarded_lock_x = 0x10,
			billboarded_lock_y = 0x20,
			billboarded_lock_z = 0x40,
			camera_anchored = 0x80,
			bone = 0x100,
			light = 0x200,
			object = 0x400,
			attachment = 0x800,
			emitter = 0x1000,
			collision_shape = 0x2000,
			ribbon_emitter = 0x4000,
			// if_particle_emitter : emitter_uses_mdl,
			unshaded = 0x8000,
			// if_particle_emitter : emitter_uses_tga,
			sort_primitives_far_z = 0x10000,
			line_emitter = 0x20000,
			unfogged = 0x40000,
			model_space = 0x80000,
			xy_quad = 0x100000
		};
	};

	export struct Extent {
		float bounds_radius;
		glm::vec3 minimum;
		glm::vec3 maximum;

		Extent() = default;
		explicit Extent(BinaryReader& reader) {
			bounds_radius = reader.read<float>();
			minimum = reader.read<glm::vec3>();
			maximum = reader.read<glm::vec3>();
		}

		void save(BinaryWriter& writer) const {
			writer.write<float>(bounds_radius);
			writer.write<glm::vec3>(minimum);
			writer.write<glm::vec3>(maximum);
		}
	};

	export struct Sequence {
		std::string name;
		uint32_t start_frame;
		uint32_t end_frame;
		float movespeed;
		uint32_t flags;
		float rarity;
		uint32_t sync_point;
		Extent extent;

		enum Flags {
			looping,
			non_looping
		};
	};

	export struct Geoset {
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
	    /// LODs are unused by the WC3 engine afaik
		uint32_t lod;
		std::string lod_name;
	    /// The extent of this geoset not accounting for animations (?)
		Extent extent;

		/// One per sequence?
		std::vector<Extent> sequence_extents;

		std::vector<glm::vec4> tangents;
		/// vertices.size() * 4 bone indices, each a byte, 4 weights, each a byte
		std::vector<uint8_t> skin;
		/// We only support one uv set
		std::vector<std::vector<glm::vec2>> uv_sets;
	};

	export struct GeosetAnimation {
		float alpha;
		uint32_t flags;
		glm::vec3 color; // BGR, not RGB
		uint32_t geoset_id;

		TrackHeader<float> KGAO;
		TrackHeader<glm::vec3> KGAC;
	};

	export struct Texture {
		uint32_t replaceable_id;
		fs::path file_name;
		uint32_t flags;

		enum Flags {
			wrap_width = 1,
			wrap_height
		};

		bool operator==(const Texture&) const = default;
	};

	export struct Material {
		uint32_t priority_plane;
		uint32_t flags;
		std::vector<Layer> layers;

		bool operator==(const Material&) const = default;
	};

	struct Bone {
		Node node;
		int32_t geoset_id;
		int32_t geoset_animation_id;
	};

	struct Light {
		Node node;
		int type;
		int attenuation_start;
		int attenuation_end;
		glm::vec3 color;
		float intensity;
		glm::vec3 ambient_color;
		float ambient_intensity;
		float shadow_intensity;

		TrackHeader<uint32_t> KLAS;
		TrackHeader<uint32_t> KLAE;
		TrackHeader<glm::vec3> KLAC;
		TrackHeader<float> KLAI;
		TrackHeader<float> KLBI;
		TrackHeader<glm::vec3> KLBC;
		TrackHeader<float> KLAV;
	};

	struct Attachment {
		Node node;
		std::string path; // Reference to Undead, NE, or Naga birth anim
		int reserved;	  // ToDo mine meaning of reserved from Game.dll, likely strlen
		int attachment_id;

		TrackHeader<float> KATV;
	};

	// Dragon/bird death bone emitter; usually emit MDLs based
	// on "path" string, but has a setting to emit TGAs from
	// path also (In practice EmitterUsesTGA setting is almost
	// never used, in favor of ParticleEmitter2).
	struct ParticleEmitter1 {
		Node node;
		float emission_rate;
		float gravity;
		float longitude;
		float latitude;
		std::string path;
		int reserved; // ToDo mine meaning, same as Attachment's reserved?
		float life_span;
		float speed;

		TrackHeader<float> KPEE;
		TrackHeader<float> KPEG;
		TrackHeader<float> KPLN;
		TrackHeader<float> KPLT;
		TrackHeader<float> KPEL;
		TrackHeader<float> KPES;
		TrackHeader<float> KPEV;
	};

	/*
	ParticleEmitter2: Texture only advanced "v2" emitter (99.9% of use cases)

	The "Tail" mode emits elongated particles, and the "Head"
	mode emits perfect square shapes. If you think about
	something like frost armor's damage that looks like a "strike,"
	this is a tail particle. The Wisp is another example. Disenchant
	is another.
	This can also be used where all elongated particles are going
	the same direction (based on latitude settings), such as the
	vertical lines in the yellow "heal" spell effect.

	The "Head" mode would be used for the snowflakes in frost armor's
	damage. It is also used in the water spray from the base of the
	Water Elemental. We say in comments we want to support the bubble geyser.
	That is a "Head" mode particle emitting a bubble
	texture, and a second one with XYQuad setting (flat, not facing camera) emitting the
	ripples above the water.

	There is also a "Both" mode that will emit both head and tail (when head_or_tail=2).

	--
	Each ParticleEmitter2 is a rectangular area specified by length and width that
	can be hand drawn in 3DS max, but many of them choose to forgo use of the rectangle,
	make it very small, and then use latitude settings to emit outward in all directions
	randomly, effectively making a point emitter (like NE Wisp).
	I seem to recall that variation is the random speed variation (when zero,
	all particles would move with equal speed, modified downward by gravity setting
	as an acceleration ).
	*/
	struct ParticleEmitter2 {
		Node node;
		float speed;
		float variation;
		float latitude;
		float gravity;
		float life_span;
		float emission_rate;
		float length;
		float width;
		uint32_t filter_mode;
		uint32_t rows; // for Textures\Clouds8x8 files
		uint32_t columns;
		uint32_t head_or_tail;
		float tail_length;
		float time_middle;

		glm::vec3 start_segment_color;
		glm::vec3 middle_segment_color;
		glm::vec3 end_segment_color;
		glm::u8vec3 segment_alphas;
		glm::vec3 segment_scaling;
		glm::uvec3 head_intervals;
		glm::uvec3 head_decay_intervals;
		glm::uvec3 tail_intervals;
		glm::uvec3 tail_decay_intervals;
		uint32_t texture_id;
		uint32_t squirt;
		uint32_t priority_plane;
		uint32_t replaceable_id; // for Wisp team color particles

		TrackHeader<float> KP2S;
		TrackHeader<float> KP2R;
		TrackHeader<float> KP2L;
		TrackHeader<float> KP2G;
		TrackHeader<float> KP2E;
		TrackHeader<float> KP2N;
		TrackHeader<float> KP2W;
		TrackHeader<float> KP2V;
	};

	struct RibbonEmitter {
		Node node;
		float height_above;
		float height_below;
		float alpha;
		glm::vec3 color;
		float life_span;
		uint32_t texture_slot;
		uint32_t emission_rate;
		uint32_t rows;
		uint32_t columns;
		uint32_t material_id; // note: not a texture id, avoids need for filtermode field like PE2
		float gravity;

		TrackHeader<float> KRHA;
		TrackHeader<float> KRHB;
		TrackHeader<float> KRAL;
		TrackHeader<glm::vec3> KRCO;
		TrackHeader<uint32_t> KRTX;
		TrackHeader<float> KRVS;
	};

	/*
	 EventObjects:
	 The type of sound or spawned effect is determined by node name
	 and SLK table lookup. The default World Editor ignores EventObjects
	 entirely, so they are only viewable in game. Even when you activate
	 the GEM setting in the Terrain Editor and listen for unit death sounds
	 when deleting them, the World Editor plays the sound file from the soundset
	 information despite the game playing the sound file from the EventObject.

	 Every EventObject's name is typically 8 characters. It usually starts with:
		SPN to spawn a model file from "Splats\SpawnData.slk"
		   - Example: illidan footprints, blood particle emitters
		SPLT to spawn a ground texture from "Splats\SplatData.slk"
			 - Example: blood ground texture on unit death
		FPT to spawn a footprint also from "Splats\SplatData.slk"
		   - It is possible that FPT animates differently,
			 such as only shows on certain terrain?? (ToDo research if needed)
		   - Some FPT entries make situational sounds, such as spiders walking on metallic
			 tiles (icecrown tileset bricks/runes) if memory serves? (ToDo research if needed)
		UBR to spawn a temporary uber splat from "Splats\UberSplatData.slk"
		   - Example: Several buildings, when they die, use an UBR tag to create a crater
			   style ground texture. Flamestrike also uses this style of model tag
			 to spawn its ground texture.
		SND to play a sound from "UI\SoundInfo\AnimLookups.slk" (unit death and spell sounds)

	 The 4 last characters of the 8-character name will be the 4-digit rawcode
	 SLK table lookup key within the particular table being used.
	 RoC Beta had 5-digit rawcodes present in the UberSplatData.slk
	 that I did not research. They were probably just a different way
	 to store terrain information; I do not know if they were used in any
	 model files. The uber splat table is also used by the World Editor
	 for building Ground Textures, so it is possible that the World Editor
	 accepts the five letter codes and this allowed Blizzard to test the
	 different per-tileset variations of the entries.

	 For 3-letter table names, like FPT, the 4th character is often "x" or "y",
	 presumably a redundant indicator for left or right, although both
	 flipped versions of most footprint textures exist and are loaded with
	 separate table entries.

	 Tags like "SNDxPOOP" might exist for an SLK table entry we do not have that is only
	 found in custom environments. Although modern maps cannot easily override
	 these tables, old MPQ mods on historic versions of the game exist with
	 fan created table entries. For example, in the TToR Mod, when you
	 kill a Balrog it makes a very loud custom noise that is embedded into
	 the model via a custom EventObject. So we must allow invalid entries without crashing.
	 If the "times" vector is empty, then War3 does not load the model, and we can consider
	 it invalid.
	*/
	struct EventObject {
		Node node;
		int global_sequence_id; // signed, -1 to specify "none"
		std::vector<uint32_t> times;
	};

	/*
	I was pretty sure that in the old days, not having a CollisionShape meant that
	a unit was not able to be selected in-game. However, at some point,
	I think they patched it so that it usually always works. Might've been
	the 2009 patch cycle, could have been TFT.

	So at this point I've seen some models that didn't have collision shapes that
	worked fine, but we need to parse them for rendering since they are legal
	nodes and could technically be a parent of another node.

	(They are used for ray intersection bounding cues, not for in-game "collision")
	(World editor doesn't use them and uses MODL/GEOS for selection and bounding cues, which
	is also why you don't ever have a doodad model that cant be clicked on in WE,
	even though some doodad models can't be clicked on easily in-game)

	Voidwalker's attack animation animates his CollisionShape to float outside
	of his center and DracoL1ch said that for DotA at some point he had to
	replace the Voidwalker model because of user complaints where the Voidwalker
	model had "invincibility frames" effectively, where it could not be attacked,
	because the CollisionShape had floated away.
	*/
	struct CollisionShape {
		enum class Shape {
			Box = 0,	 // 2 verts
			Plane = 1,	 // 2 verts
			Sphere = 2,	 // 1 verts
			Cylinder = 3 // 2 vert
		};

		Node node;
		Shape type;
		glm::vec3 vertices[2]; // sometimes only 1 is used
		float radius;		   // used for sphere/cylinder
	};

	struct FaceFX {
		std::string name;
		fs::path path;
	};

	struct CornEmitter {
		Node node;
		std::vector<uint8_t> data; // Just store it so we can save it again
	};

	struct Camera {
		std::vector<uint8_t> data; // Just store it so we can save it again
	};

	struct TextureAnimation {
		std::vector<uint8_t> data; // Just store it so we can save it again
	};

	export class MDX {
	public:
		int unique_tracks = 0;

		static constexpr uint32_t LATEST_MDX_VERSION = 1200;

		uint32_t version = LATEST_MDX_VERSION;
		std::string name;
		std::string animation_filename;
		Extent extent;
		uint32_t blend_time;

		std::string face_target;
		std::string face_path;

		std::vector<Geoset> geosets;
		std::vector<Sequence> sequences;
		std::vector<uint32_t> global_sequences;
		std::vector<GeosetAnimation> animations;
		std::vector<Bone> bones;
		std::vector<Material> materials;
		std::vector<Texture> textures;
		std::vector<Light> lights;
		std::vector<Node> help_bones;
		std::vector<Attachment> attachments;
		std::vector<glm::vec3> pivots;
		std::vector<ParticleEmitter1> emitters1;
		std::vector<ParticleEmitter2> emitters2;
		std::vector<RibbonEmitter> ribbons;
		std::vector<EventObject> event_objects;
		std::vector<CollisionShape> collision_shapes;
		std::vector<CornEmitter> corn_emitters;
		std::vector<FaceFX> facefxes;
		std::vector<Camera> cameras;
		std::vector<float> bind_poses;
		std::vector<TextureAnimation> texture_animations;

	private:
		void load(BinaryReader& reader);

	public:
		MDX() = default;

		explicit MDX(BinaryReader& reader) {
			load(reader);
		}

		[[nodiscard]] BinaryWriter save() const;

		std::string to_mdl();
		static result<MDX, std::string> from_mdl(std::string_view mdl);

		void validate();

		void merge_with(const MDX& mdx, const glm::mat4& transform);

		static std::vector<glm::u8vec4> matrix_groups_as_skin_weights(const Geoset& geoset);

		struct OptimizationStats {
			size_t materials_removed = 0;
			size_t textures_removed = 0;

			size_t constant_tracks = 0;
			size_t constant_tracks_removed = 0;
			size_t linear_tracks = 0;
			size_t linear_tracks_removed = 0;
			size_t hermite_tracks = 0;
			size_t hermite_tracks_removed = 0;
			size_t bezier_tracks = 0;
			size_t bezier_tracks_removed = 0;
		};

		OptimizationStats optimize(float max_error);
		MDX& deduplicate_textures();
		MDX& deduplicate_materials();
		MDX& deduplicate_geosets();

		MDX& calculate_extents();

		template<std::invocable<Node&> Func>
		void for_each_node(const Func F) {
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

			for (auto& i : event_objects) {
				F(i.node);
			}

			for (auto& i : collision_shapes) {
				F(i.node);
			}

			for (auto& i : corn_emitters) {
				F(i.node);
			}
		}

		template<typename Func>
		requires std::invocable<Func, TrackHeader<float>&>
		and std::invocable<Func, TrackHeader<uint32_t>&>
		and std::invocable<Func, TrackHeader<glm::vec3>&>
		and std::invocable<Func, TrackHeader<glm::quat>&>
		void for_each_track(const Func F) {
			for_each_node([&](Node& node) {
				F(node.KGRT);
				F(node.KGTR);
				F(node.KGSC);
			});

			for (auto& i : animations) {
				F(i.KGAC);
				F(i.KGAO);
			}

			for (auto& i : attachments) {
				F(i.KATV);
			}

			for (auto& i : emitters1) {
				F(i.KPEE);
				F(i.KPEG);
			}

			for (auto& i : emitters2) {
				F(i.KP2E);
				F(i.KP2G);
				F(i.KP2R);
				F(i.KP2W);
				F(i.KP2N);
			}

			for (auto& i : materials) {
				for (auto& j : i.layers) {
					F(j.KMTA);
					F(j.KMTE);
					F(j.KFC3);
					F(j.KFCA);
					F(j.KFTC);
					for (auto& k : j.texturess) {
						F(k.KMTF);
					}
				}
			}

			for (auto& i : lights) {
				F(i.KLAS);
				F(i.KLAE);
				F(i.KLAC);
				F(i.KLAI);
				F(i.KLBI);
				F(i.KLBC);
				F(i.KLAV);
			}

			for (auto& i : ribbons) {
				F(i.KRHA);
				F(i.KRHB);
				F(i.KRAL);
				F(i.KRCO);
				F(i.KRTX);
				F(i.KRVS);
			}
		}
	};
} // namespace mdx

// All our hashes
// C++ really needs a derive macro for this
template <typename T>
void hash_combine(std::size_t& seed, const T& v) {
	seed ^= std::hash<T>{}(v) + 0x9e3779b97f4a7c15ULL + (seed<<6) + (seed>>2);
}

template <typename T>
struct hash_vector {
	std::size_t operator()(const std::vector<T>& vec) const {
		std::size_t h = 0;
		for (const auto& item : vec)
			hash_combine(h, item);  // assumes std::hash<T> is defined
		return h;
	}
};

namespace std {
	template <>
	struct hash<glm::vec3> {
		std::size_t operator()(const glm::vec3& v) const {
			std::size_t h = 0;
			hash_combine(h, v.x);
			hash_combine(h, v.y);
			hash_combine(h, v.z);
			return h;
		}
	};

	template <typename T>
	struct std::hash<mdx::Track<T>> {
		std::size_t operator()(const mdx::Track<T>& t) const {
			std::size_t h = 0;
			hash_combine(h, t.frame);
			hash_combine(h, t.value);
			hash_combine(h, t.inTan);
			hash_combine(h, t.outTan);
			return h;
		}
	};

	template <typename T>
	struct std::hash<mdx::TrackHeader<T>> {
		std::size_t operator()(const mdx::TrackHeader<T>& th) const {
			std::size_t h = 0;
			hash_combine(h, static_cast<int>(th.interpolation_type));
			hash_combine(h, th.global_sequence_ID);
			for (const auto& track : th.tracks)
				hash_combine(h, track);
			return h;
		}
	};

	template <>
	struct hash<mdx::LayerTexture> {
		std::size_t operator()(const mdx::LayerTexture& lt) const {
			std::size_t h = 0;
			hash_combine(h, lt.id);
			hash_combine(h, lt.KMTF);
			return h;
		}
	};


	template <>
	struct hash<mdx::Layer> {
		std::size_t operator()(const mdx::Layer& l) const {
			std::size_t h = 0;
			hash_combine(h, l.blend_mode);
			hash_combine(h, l.shading_flags);
			hash_combine(h, l.texture_animation_id);
			hash_combine(h, l.coord_id);
			hash_combine(h, l.alpha);
			hash_combine(h, l.emissive_gain);
			hash_combine(h, l.fresnel_color);
			hash_combine(h, l.fresnel_opacity);
			hash_combine(h, l.fresnel_team_color);
			hash_combine(h, l.hd);
			for (const auto& tex : l.texturess)
				hash_combine(h, tex);
			hash_combine(h, l.KMTA);
			hash_combine(h, l.KMTE);
			hash_combine(h, l.KFC3);
			hash_combine(h, l.KFCA);
			hash_combine(h, l.KFTC);
			return h;
		}
	};

	// template<typename T>
	// struct hash<std::vector<T>> {
	// 	std::size_t operator()(const std::vector<T>& vec) const {
	// 		std::size_t h = 0;
	// 		for (const auto& item : vec)
	// 			hash_combine(h, item);  // assumes std::hash<T> exists
	// 		return h;
	// 	}
	// };


	template<>
	struct std::hash<mdx::Material>
	{
		std::size_t operator()(const mdx::Material& s) const noexcept
		{
			std::size_t h1 = std::hash<uint32_t>{}(s.priority_plane);
			std::size_t h2 = std::hash<uint32_t>{}(s.flags);
			std::size_t h3 = hash_vector<mdx::Layer>{}(s.layers);

			// std::size_t h3 = std::hash<std::vector<mdx::Layer>>{}(s.layers);
			return h1 ^ (h2 << 1) ^(h3 << 1); // or use boost::hash_combine
		}
	};

	template<>
	struct std::hash<mdx::Texture>
	{
		std::size_t operator()(const mdx::Texture& s) const noexcept
		{
			std::size_t h1 = std::hash<fs::path>{}(s.file_name);
			std::size_t h2 = std::hash<uint32_t>{}(s.flags);
			std::size_t h3 = std::hash<uint32_t>{}(s.replaceable_id);
			return h1 ^ (h2 << 1) ^(h3 << 1); // or use boost::hash_combine
		}
	};
}