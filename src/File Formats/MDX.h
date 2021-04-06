#pragma once

#include <functional>
#include <filesystem>
namespace fs = std::filesystem;
#include <unordered_map>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "BinaryReader.h"
#include "BinaryWriter.h"

namespace mdx {
	extern std::unordered_map<int, std::string> replacable_id_to_texture;

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

	template <typename T>
	struct Track {
		int32_t frame;
		T value;
		T inTan;
		T outTan;
	};

	template <typename T>
	struct TrackHeader {
		int32_t interpolation_type = 0;
		int32_t global_sequence_ID = -1;
		std::vector<Track<T>> tracks;

		int id = -1; // Used to track each individual track for animation purposes

		TrackHeader() = default;
		explicit TrackHeader(BinaryReader& reader, int track_id) {
			const uint32_t tracks_count = reader.read<uint32_t>();
			interpolation_type = reader.read<int32_t>();
			global_sequence_ID = reader.read<int32_t>();
			id = track_id;

			tracks.reserve(tracks_count);
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
				if (interpolation_type > 1) {
					writer.write<T>(i.inTan);
					writer.write<T>(i.outTan);
				}
			}
		}
	};

	struct Layer {
		uint32_t blend_mode;
		uint32_t shading_flags;
		uint32_t texture_id;
		uint32_t texture_animation_id;
		uint32_t coord_id;
		float alpha;

		float emissive_gain;
		glm::vec3 fresnel_color;
		float fresnel_opacity;
		float fresnel_team_color;

		TrackHeader<uint32_t> KMTF;
		TrackHeader<float> KMTA;
		TrackHeader<float> KMTE;
		TrackHeader<glm::vec3> KFC3;
		TrackHeader<float> KFCA;
		TrackHeader<float> KFTC;
	};

	struct Node {
		Node() = default;
		explicit Node(BinaryReader& reader, int& unique_tracks);
		void save(BinaryWriter& writer) const;

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
			//if_particle_emitter : emitter_uses_mdl,
			//if_particle_emitter_2 : unshaded = 0x8000,
			//if_particle_emitter : emitter_uses_tga,
			//if_particle_emitter_2 : sort_primitives_far_z = 0x10000,
			line_emitter = 0x20000,
			unfogged = 0x40000,
			model_space = 0x80000,
			xy_quad = 0x100000
		};
	};

	struct Extent {
		float bounds_radius;
		glm::vec3 minimum;
		glm::vec3 maximum;

		Extent() = default;
		explicit Extent(BinaryReader& reader);

		void save(BinaryWriter& writer) const;
	};

	struct Sequence {
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
		uint32_t lod;
		std::string lod_name;
		Extent extent;

		std::vector<Extent> extents;

		std::vector<glm::vec4> tangents;
		std::vector<uint8_t> skin;

		using TextureCoordinateSet = std::vector<glm::vec2>;
		std::vector<TextureCoordinateSet> texture_coordinate_sets;
	};

	struct GeosetAnimation {
		float alpha;
		uint32_t flags;
		glm::vec3 color;
		uint32_t geoset_id;

		TrackHeader<float> KGAO;
		TrackHeader<glm::vec3> KGAC;
	};

	struct Texture {
		uint32_t replaceable_id;
		fs::path file_name;
		uint32_t flags;
	};

	struct Material {
		uint32_t priority_plane;
		uint32_t flags;
		std::string shader_name;
		std::vector<Layer> layers;
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
		//float segment_color[3][3]; // rows [Begin, Middle, End], column is color
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

	enum class CollisionShapeType {
		Box = 0,	 // 2 verts
		Plane = 1,	 // 2 verts
		Sphere = 2,	 // 1 verts
		Cylinder = 3 // 2 vert
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
		Node node;
		CollisionShapeType type;
		glm::vec3 vertices[2]; // sometimes only 1 is used
		float radius;		  // used for sphere/cylinder
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

	class MDX {
		int version;
		std::string name;
		std::string animation_filename;
		Extent extent;
		uint32_t blend_time;

		std::string face_target;
		std::string face_path;

		void read_GEOS_chunk(BinaryReader& reader);
		void read_MTLS_chunk(BinaryReader& reader);
		void read_SEQS_chunk(BinaryReader& reader);
		void read_GLBS_chunk(BinaryReader& reader);
		void read_GEOA_chunk(BinaryReader& reader);
		void read_BONE_chunk(BinaryReader& reader);
		void read_TEXS_chunk(BinaryReader& reader);
		void read_LITE_chunk(BinaryReader& reader);
		void read_HELP_chunk(BinaryReader& reader);
		void read_ATCH_chunk(BinaryReader& reader);
		void read_PIVT_chunk(BinaryReader& reader);
		void read_PREM_chunk(BinaryReader& reader);
		void read_PRE2_chunk(BinaryReader& reader);
		void read_RIBB_chunk(BinaryReader& reader);
		void read_EVTS_chunk(BinaryReader& reader);
		void read_CLID_chunk(BinaryReader& reader);
		void read_CORN_chunk(BinaryReader& reader);
		void read_CAMS_chunk(BinaryReader& reader);
		void read_BPOS_chunk(BinaryReader& reader);
		void read_TXAN_chunk(BinaryReader& reader);

		void write_GEOS_chunk(BinaryWriter& writer) const;
		void write_MTLS_chunk(BinaryWriter& writer) const;
		void write_SEQS_chunk(BinaryWriter& writer) const;
		void write_GLBS_chunk(BinaryWriter& writer) const;
		void write_GEOA_chunk(BinaryWriter& writer) const;
		void write_BONE_chunk(BinaryWriter& writer) const;
		void write_TEXS_chunk(BinaryWriter& writer) const;
		void write_LITE_chunk(BinaryWriter& writer) const;
		void write_HELP_chunk(BinaryWriter& writer) const;
		void write_ATCH_chunk(BinaryWriter& writer) const;
		void write_PIVT_chunk(BinaryWriter& writer) const;
		void write_PREM_chunk(BinaryWriter& writer) const;
		void write_PRE2_chunk(BinaryWriter& writer) const;
		void write_RIBB_chunk(BinaryWriter& writer) const;
		void write_EVTS_chunk(BinaryWriter& writer) const;
		void write_CLID_chunk(BinaryWriter& writer) const;
		void write_CORN_chunk(BinaryWriter& writer) const;
		void write_CAMS_chunk(BinaryWriter& writer) const;
		void write_BPOS_chunk(BinaryWriter& writer) const;
		void write_TXAN_chunk(BinaryWriter& writer) const;

	  public:
		explicit MDX(BinaryReader& reader);
		void load(BinaryReader& reader);
		void save(const fs::path& path);

		void validate();
		void optimize();

		int unique_tracks = 0;

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
		std::vector<EventObject> eventObjects;
		std::vector<CollisionShape> collisionShapes;
		std::vector<CornEmitter> corn_emitters;

		std::vector<Camera> cameras;
		std::vector<float> bind_poses;
		std::vector<TextureAnimation> texture_animations;

		void forEachNode(const std::function<void(Node&)>& lambda);
	};
} // namespace mdx