module;

#define GLM_FORCE_CXX17
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <functional>
#include <filesystem>
#include <unordered_map>
#include <string_view>
#include <fstream>
#include <fmt/format.h>

export module MDX;

namespace fs = std::filesystem;

import BinaryReader;
import BinaryWriter;

namespace mdx {
	export extern const std::unordered_map<int, std::string> replacable_id_to_texture{
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
	};

	export template <typename T>
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
			for (size_t i = 0; i < tracks_count; i++) {
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

	export struct Layer {
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

		enum ShadingFlags {
			unshaded = 1,
			sphere_environment_map = 2,
			uknown1 = 4,
			unknown2 = 8,
			two_sided = 16,
			unfogged = 32,
			no_depth_test = 64,
			no_depth_set = 128
		};
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
					fmt::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
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
		uint32_t lod;
		std::string lod_name;
		Extent extent;

		std::vector<Extent> extents;

		std::vector<glm::vec4> tangents;
		std::vector<uint8_t> skin;

		using TextureCoordinateSet = std::vector<glm::vec2>;
		std::vector<TextureCoordinateSet> texture_coordinate_sets;
	};

	export struct GeosetAnimation {
		float alpha;
		uint32_t flags;
		glm::vec3 color;
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
	};

	export struct Material {
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
		int version;
		std::string name;
		std::string animation_filename;
		Extent extent;
		uint32_t blend_time;

		std::string face_target;
		std::string face_path;

		void read_GEOS_chunk(BinaryReader& reader) {
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

		void read_MTLS_chunk(BinaryReader& reader) {
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
							fmt::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
						}
					}

					material.layers.push_back(std::move(layer));
				}

				materials.push_back(std::move(material));
			}
		}

		void read_SEQS_chunk(BinaryReader& reader) {
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

		void read_GEOA_chunk(BinaryReader& reader) {
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
						fmt::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
					}
				}

				animations.push_back(std::move(animation));
			}
		}

		void read_BONE_chunk(BinaryReader& reader) {
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

		void read_TEXS_chunk(BinaryReader& reader) {
			const uint32_t size = reader.read<uint32_t>();
			for (size_t i = 0; i < size / 268; i++) {
				Texture texture;
				texture.replaceable_id = reader.read<uint32_t>();
				texture.file_name = reader.read_string(260);
				texture.flags = reader.read<uint32_t>();
				textures.push_back(std::move(texture));
			}
		}

		void read_GLBS_chunk(BinaryReader& reader) {
			const uint32_t size = reader.read<uint32_t>();
			global_sequences = reader.read_vector<uint32_t>(size / 4);
		}

		void read_LITE_chunk(BinaryReader& reader) {
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
						fmt::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
					}
				}
				lights.push_back(std::move(light));
			}
		}

		void read_HELP_chunk(BinaryReader& reader) {
			const size_t reader_pos = reader.position;
			const uint32_t size = reader.read<uint32_t>();
			while (reader.position < reader_pos + size) {
				help_bones.push_back(Node(reader, unique_tracks));
			}
		}

		void read_ATCH_chunk(BinaryReader& reader) {
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
						fmt::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
					}
				}
				attachments.push_back(std::move(attachment));
			}
		}

		void read_PIVT_chunk(BinaryReader& reader) {
			const size_t reader_pos = reader.position;
			const uint32_t size = reader.read<uint32_t>();

			pivots = reader.read_vector<glm::vec3>(size / 12);
		}

		void read_PREM_chunk(BinaryReader& reader) {
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
						fmt::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
					}
				}
				emitters1.push_back(std::move(emitter));
			}
		}

		void read_PRE2_chunk(BinaryReader& reader) {
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
						fmt::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
					}
				}
				emitters2.push_back(std::move(emitter2));
			}
		}

		void read_RIBB_chunk(BinaryReader& reader) {
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
						fmt::print("Unknown track tag {}\n", static_cast<uint32_t>(tag));
					}
				}
				ribbons.push_back(std::move(emitter));
			}
		}

		void read_EVTS_chunk(BinaryReader& reader) {
			const size_t reader_pos = reader.position;
			const uint32_t size = reader.read<uint32_t>();

			while (reader.position < reader_pos + size) {
				EventObject evt;
				evt.node = Node(reader, unique_tracks);
				reader.advance(4); // read KEVT
				uint32_t count = reader.read<uint32_t>();
				evt.global_sequence_id = reader.read<int32_t>(); // signed
				evt.times = reader.read_vector<uint32_t>(count);
				event_objects.push_back(std::move(evt));
			}
		}

		void read_CLID_chunk(BinaryReader& reader) {
			const size_t reader_pos = reader.position;
			const uint32_t size = reader.read<uint32_t>();

			while (reader.position < reader_pos + size) {
				CollisionShape shape;
				shape.node = Node(reader, unique_tracks);
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
				collision_shapes.push_back(std::move(shape));
			}
		}

		void read_CORN_chunk(BinaryReader& reader) {
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

		void read_CAMS_chunk(BinaryReader& reader) {
			const size_t reader_pos = reader.position;
			const uint32_t size = reader.read<uint32_t>();

			while (reader.position < reader_pos + size) {
				Camera camera;
				const uint32_t inclusive_size = reader.read<uint32_t>();

				camera.data = reader.read_vector<uint8_t>(inclusive_size - 4);
				cameras.push_back(std::move(camera));
			}
		}

		void read_BPOS_chunk(BinaryReader& reader) {
			const uint32_t size = reader.read<uint32_t>();
			bind_poses = reader.read_vector<float>(reader.read<uint32_t>() * 12);
		}

		void read_TXAN_chunk(BinaryReader& reader) {
			const size_t reader_pos = reader.position;
			const uint32_t size = reader.read<uint32_t>();

			while (reader.position < reader_pos + size) {
				TextureAnimation animation;
				const uint32_t inclusive_size = reader.read<uint32_t>();

				animation.data = reader.read_vector<uint8_t>(inclusive_size - 4);
				texture_animations.push_back(std::move(animation));
			}
		}

		void read_FAFX_chunk(BinaryReader& reader) {
			const uint32_t size = reader.read<uint32_t>();
			for (size_t i = 0; i < size / 340; i++) {
				FaceFX facefx;
				facefx.name = reader.read_string(80);
				facefx.path = reader.read_string(260);
				facefxes.push_back(std::move(facefx));
			}
		}

		void write_GEOS_chunk(BinaryWriter& writer) const {
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
				const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - geoset_index);
				std::memcpy(writer.buffer.data() + geoset_index, &temporary, 4);
			}
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_MTLS_chunk(BinaryWriter& writer) const {
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

					const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - layer_index);
					std::memcpy(writer.buffer.data() + layer_index, &temporary, 4);
				}
				const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - material_index);
				std::memcpy(writer.buffer.data() + material_index, &temporary, 4);
			}
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_SEQS_chunk(BinaryWriter& writer) const {
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

		void write_GLBS_chunk(BinaryWriter& writer) const {
			if (global_sequences.empty()) {
				return;
			}

			writer.write(ChunkTag::GLBS);
			writer.write<uint32_t>(global_sequences.size() * 4);
			writer.write_vector(global_sequences);
		}

		void write_GEOA_chunk(BinaryWriter& writer) const {
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

				const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - geoset_index);
				std::memcpy(writer.buffer.data() + geoset_index, &temporary, 4);
			}
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_BONE_chunk(BinaryWriter& writer) const {
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

		void write_TEXS_chunk(BinaryWriter& writer) const {
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

		void write_LITE_chunk(BinaryWriter& writer) const {
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

				const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - light_index);
				std::memcpy(writer.buffer.data() + light_index, &temporary, 4);
			}
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_HELP_chunk(BinaryWriter& writer) const {
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
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_ATCH_chunk(BinaryWriter& writer) const {
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

				const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - attachment_index);
				std::memcpy(writer.buffer.data() + attachment_index, &temporary, 4);
			}
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_PIVT_chunk(BinaryWriter& writer) const {
			if (pivots.empty()) {
				return;
			}

			writer.write(ChunkTag::PIVT);
			writer.write<uint32_t>(pivots.size() * 12);
			for (const auto& pivot : pivots) {
				writer.write<glm::vec3>(pivot);
			}
		}

		void write_PREM_chunk(BinaryWriter& writer) const {
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

				const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - emitter_index);
				std::memcpy(writer.buffer.data() + emitter_index, &temporary, 4);
			}
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_PRE2_chunk(BinaryWriter& writer) const {
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

				const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - emitter_index);
				std::memcpy(writer.buffer.data() + emitter_index, &temporary, 4);
			}
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_RIBB_chunk(BinaryWriter& writer) const {
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

				const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - ribbon_index);
				std::memcpy(writer.buffer.data() + ribbon_index, &temporary, 4);
			}
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_EVTS_chunk(BinaryWriter& writer) const {
			if (event_objects.empty()) {
				return;
			}

			writer.write(ChunkTag::EVTS);
			// Write temporary zero, remember location
			const size_t inclusive_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			for (const auto& event_object : event_objects) {
				event_object.node.save(writer);
				writer.write_string("KEVT");
				writer.write<uint32_t>(event_object.times.size());
				writer.write<int32_t>(event_object.global_sequence_id);
				writer.write_vector(event_object.times);
			}
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_CLID_chunk(BinaryWriter& writer) const {
			if (collision_shapes.empty()) {
				return;
			}

			writer.write(ChunkTag::CLID);
			// Write temporary zero, remember location
			const size_t inclusive_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			for (const auto& shape : collision_shapes) {
				shape.node.save(writer);
				writer.write<uint32_t>(static_cast<uint32_t>(shape.type));

				if (shape.type == CollisionShape::Shape::Sphere) {
					writer.write<glm::vec3>(shape.vertices[0]);
				} else {
					writer.write<glm::vec3>(shape.vertices[0]);
					writer.write<glm::vec3>(shape.vertices[1]);
				}
				if (shape.type == CollisionShape::Shape::Sphere || shape.type == CollisionShape::Shape::Cylinder) {
					writer.write<float>(shape.radius);
				}
			}
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_CORN_chunk(BinaryWriter& writer) const {
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

		void write_CAMS_chunk(BinaryWriter& writer) const {
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

				const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - camera_index);
				std::memcpy(writer.buffer.data() + camera_index, &temporary, 4);
			}
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_BPOS_chunk(BinaryWriter& writer) const {
			if (bind_poses.empty()) {
				return;
			}

			writer.write(ChunkTag::BPOS);
			writer.write<uint32_t>(4 + bind_poses.size() * 4);
			writer.write<uint32_t>(bind_poses.size() / 12);
			writer.write_vector(bind_poses);
		}

		void write_TXAN_chunk(BinaryWriter& writer) const {
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

				const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - texture_animation_index);
				std::memcpy(writer.buffer.data() + texture_animation_index, &temporary, 4);
			}
			const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
			std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
		}

		void write_FAFX_chunk(BinaryWriter& writer) const {
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

		void load(BinaryReader& reader) {
			const std::string magic_number = reader.read_string(4);
			if (magic_number != "MDLX") {
				fmt::print("Incorrect file magic number, expected MDLX but got {}\n", magic_number);
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

	  public:
		explicit MDX(BinaryReader& reader) {
			load(reader);
		}
		void save(const fs::path& path) {
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

		void validate() {
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
								event_objects.size() +
								collision_shapes.size() +
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
			for (size_t i = 0; i < IDs.size(); i++) {
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

		void optimize() {
			Bone& bone = bones.front();
			auto& header = bone.node.KGTR;
			auto new_tracks = header.tracks;

			Sequence& current_sequence = sequences.front();
			// for (const auto& track : header.tracks) {
			for (size_t i = 0; i < new_tracks.size(); i++) {
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

		/// A minimal utility wrapper around an std::string that manages newlines, indentation and closing braces
		struct MDLWriter {
			std::string mdl;
			size_t current_indentation = 0;

			/// Writes a line and automatically handles indentation and the newline character
			void write_line(std::string_view line) {
				for (size_t i = 0; i < current_indentation; i++) {
					mdl += '\t';
				}
				mdl += line;
				mdl += '\n';
			}

			template <typename T>
			void write_track(const TrackHeader<T>& track_header, std::string name, T static_value) {
				if (track_header.tracks.empty()) {
					if constexpr (std::is_same_v<T, glm::vec2>) {
						write_line(fmt::format("static {} {{ {}, {} }},", name, static_value.x, static_value.y));
					} else if constexpr (std::is_same_v<T, glm::vec3>) {
						write_line(fmt::format("static {} {{ {}, {}, {} }},", name, static_value.x, static_value.y, static_value.z));
					} else if constexpr (std::is_same_v<T, glm::quat>) {
						write_line(fmt::format("static {} {{ {}, {}, {}, {} }},", name, static_value.x, static_value.y, static_value.z, static_value.w));
					} else {
						write_line(fmt::format("static {} {},", name, static_value));
					}

					// write_line(fmt::format("static {} {},", name, static_value));
				} else {
					start_group(name, [&]() {
						switch (track_header.interpolation_type) {
							case 0:
								write_line("DontInterp,");
								break;
							case 1:
								write_line("Linear,");
								break;
							case 2:
								write_line("Hermite,");
								break;
							case 3:
								write_line("Bezier,");
								break;
						}

						write_line(fmt::format("GlobalSeqId {},", track_header.global_sequence_ID));

						for (const auto& track : track_header.tracks) {
							if constexpr (std::is_same_v<T, glm::vec2>) {
								write_line(fmt::format("{}: {{ {}, {} }},", track.frame, track.value.x, track.value.y));
							} else if constexpr (std::is_same_v<T, glm::vec3>) {
								write_line(fmt::format("{}: {{ {}, {}, {} }},", track.frame, track.value.x, track.value.y, track.value.z));
							} else if constexpr (std::is_same_v<T, glm::quat>) {
								write_line(fmt::format("{}: {{ {}, {}, {}, {} }},", track.frame, track.value.x, track.value.y, track.value.z, track.value.w));
							} else {
								write_line(fmt::format("{}: {},", track.frame, track.value));
							}

							if (track_header.interpolation_type == 2 || track_header.interpolation_type == 3) {
								if constexpr (std::is_same_v<T, glm::vec2>) {
									write_line(fmt::format("InTan {{ {}, {} }},", track.inTan.x, track.inTan.y));
									write_line(fmt::format("OutTan {{ {}, {} }},", track.outTan.x, track.outTan.y));
								} else if constexpr (std::is_same_v<T, glm::vec3>) {
									write_line(fmt::format("InTan {{ {}, {}, {} }},", track.inTan.x, track.inTan.y, track.inTan.z));
									write_line(fmt::format("OutTan {{ {}, {}, {} }},", track.outTan.x, track.outTan.y, track.outTan.z));
								} else if constexpr (std::is_same_v<T, glm::quat>) {
									write_line(fmt::format("InTan {{ {}, {}, {}, {} }},", track.inTan.x, track.inTan.y, track.inTan.z, track.inTan.w));
									write_line(fmt::format("OutTan {{ {}, {}, {}, {} }},", track.outTan.x, track.outTan.y, track.outTan.z, track.outTan.w));
								} else {
									write_line(fmt::format("InTan {},", track.inTan));
									write_line(fmt::format("OutTan {},", track.outTan));
								}
							}
						}
					});
				}
			}

			void write_node(const Node& node) {
				write_line(fmt::format("ObjectId {},", node.id));
				write_line(fmt::format("Parent {},", node.parent_id));

				if (node.flags & Node::Flags::billboarded) {
					write_line("Billboarded,");
				}

				if (node.flags & Node::Flags::unfogged) {
					write_line("Unfogged,");
				}

				if (node.flags & Node::Flags::line_emitter) {
					write_line("LineEmitter,");
				}

				if (node.flags & Node::Flags::unshaded) {
					write_line("Unshaded,");
				}

				if (node.flags & Node::Flags::model_space) {
					write_line("ModelSpace,");
				}

				write_track(node.KGRT, "Rotation", glm::quat(0.f, 0.f, 0.f, 0.f));
				write_track(node.KGTR, "Translation", glm::vec3(0.0));
				write_track(node.KGSC, "Scale", glm::vec3(1.0));
			}

			template <typename T>
			void start_group(std::string name, T callback) {
				for (size_t i = 0; i < current_indentation; i++) {
					mdl += '\t';
				}
				mdl += name + " {\n";
				current_indentation += 1;
				callback();
				current_indentation -= 1;
				mdl += "}\n";
			}
		};

		std::string to_mdl() {
			MDLWriter mdl;

			mdl.start_group("Version", [&]() {
				mdl.write_line("FormatVersion 1000,");
			});

			mdl.start_group(fmt::format("Model {}", name), [&]() {
				mdl.write_line(fmt::format("BlendTime {},", blend_time));
				mdl.write_line(fmt::format("MinimumExtent {{ {}, {}, {} }},", extent.minimum.x, extent.minimum.y, extent.minimum.z));
				mdl.write_line(fmt::format("MaximumExtent {{ {}, {}, {} }},", extent.maximum.x, extent.maximum.y, extent.maximum.z));
			});

			mdl.start_group(fmt::format("Sequences {}", sequences.size()), [&]() {
				for (const auto& i : sequences) {
					mdl.start_group(fmt::format("Anim {}", i.name), [&]() {
						mdl.write_line(fmt::format("Interval {{ {}, {} }},", i.start_frame, i.end_frame));
						mdl.write_line(fmt::format("Movespeed {},", i.movespeed));
						mdl.write_line(fmt::format("SyncPoint {},", i.sync_point));

						if (i.flags & Sequence::Flags::non_looping) {
							mdl.write_line("NonLooping,");
						}

						mdl.write_line(fmt::format("Rarity {},", i.rarity));
						mdl.write_line(fmt::format("MinimumExtent {{ {}, {}, {}, }},", i.extent.minimum.x, i.extent.minimum.y, i.extent.minimum.z));
						mdl.write_line(fmt::format("MinimumExtent {{ {}, {}, {}, }},", i.extent.maximum.x, i.extent.maximum.y, i.extent.maximum.z));
						mdl.write_line(fmt::format("BoundRadius {},", i.extent.bounds_radius));
					});
				}
			});

			mdl.start_group(fmt::format("GlobalSequences {}", global_sequences.size()), [&]() {
				for (const auto& i : global_sequences) {
					mdl.write_line(fmt::format("Duration {},", i));
				}
			});

			mdl.start_group(fmt::format("Textures {}", textures.size()), [&]() {
				for (const auto& i : textures) {
					mdl.start_group("Bitmap", [&]() {
						mdl.write_line(fmt::format("Image {},", i.file_name.string()));
						mdl.write_line(fmt::format("ReplaceableId {},", i.replaceable_id));
						if (i.flags & Texture::Flags::wrap_width) {
							mdl.write_line("WrapWidth");
						}
						if (i.flags & Texture::Flags::wrap_height) {
							mdl.write_line("WrapHeight");
						}
					});
				}
			});

			mdl.start_group(fmt::format("Materials {}", materials.size()), [&]() {
				for (const auto& material : materials) {
					mdl.start_group("Material", [&]() {
						mdl.write_line(fmt::format("Shader {},", material.shader_name));

						for (const auto& layer : material.layers) {
							mdl.start_group("Layer", [&]() {
								switch (layer.blend_mode) {
									case 0:
										mdl.write_line("FilterMode None");
										break;
									case 1:
										mdl.write_line("FilterMode Transparent");
										break;
									case 2:
										mdl.write_line("FilterMode Blend");
										break;
									case 3:
										mdl.write_line("FilterMode Additive");
										break;
									case 4:
										mdl.write_line("FilterMode AddAlpha");
										break;
									case 5:
										mdl.write_line("FilterMode Modulate");
										break;
									case 6:
										mdl.write_line("FilterMode Modulate2x");
										break;
								}

								if (layer.shading_flags & Layer::ShadingFlags::unshaded) {
									mdl.write_line("Unshaded");
								}

								if (layer.shading_flags & Layer::ShadingFlags::unfogged) {
									mdl.write_line("Unfogged");
								}

								if (layer.shading_flags & Layer::ShadingFlags::no_depth_test) {
									mdl.write_line("NoDepthTest");
								}

								if (layer.shading_flags & Layer::ShadingFlags::no_depth_set) {
									mdl.write_line("NoDepthSet");
								}

								mdl.write_track(layer.KMTF, "TextureID", layer.texture_id);
								mdl.write_track(layer.KMTA, "Alpha", layer.alpha);
								mdl.write_track(layer.KMTE, "EmissiveGain", layer.emissive_gain);
								mdl.write_track(layer.KFC3, "FresnelColor", layer.fresnel_color);
								mdl.write_track(layer.KFCA, "FresnelAlpha", layer.fresnel_opacity);
								mdl.write_track(layer.KFTC, "FresnelTeamColor", layer.fresnel_team_color);
							});
						}
					});
				}
			});

			for (const auto& geoset : geosets) {
				mdl.start_group("Geoset", [&]() {
					mdl.start_group(fmt::format("Vertices {}", geoset.vertices.size()), [&]() {
						for (const auto& vertex : geoset.vertices) {
							mdl.write_line(fmt::format("{{ {}, {}, {} }},", vertex.x, vertex.y, vertex.z));
						}
					});

					mdl.start_group(fmt::format("Normals {}", geoset.normals.size()), [&]() {
						for (const auto& normal : geoset.normals) {
							mdl.write_line(fmt::format("{{ {}, {}, {} }},", normal.x, normal.y, normal.z));
						}
					});

					for (const auto& i : geoset.texture_coordinate_sets) {
						mdl.start_group(fmt::format("TVertices {}", i.size()), [&]() {
							for (const auto& uv : i) {
								mdl.write_line(fmt::format("{{ {}, {} }},", uv.x, uv.y));
							}
						});
					}

					mdl.start_group(fmt::format("Tangents {}", geoset.tangents.size()), [&]() {
						for (const auto& tangent : geoset.tangents) {
							mdl.write_line(fmt::format("{{ {}, {}, {}, {} }},", tangent.x, tangent.y, tangent.z, tangent.w));
						}
					});

					mdl.start_group(fmt::format("SkinWeights {}", geoset.skin.size() / 8), [&]() {
						for (size_t i = 0; i < geoset.skin.size() / 8; i++) {
							mdl.write_line(fmt::format("{}, {}, {}, {}, {}, {}, {}, {},",
													   geoset.skin[i * 8],
													   geoset.skin[i * 8 + 1],
													   geoset.skin[i * 8 + 2],
													   geoset.skin[i * 8 + 3],
													   geoset.skin[i * 8 + 4],
													   geoset.skin[i * 8 + 5],
													   geoset.skin[i * 8 + 6],
													   geoset.skin[i * 8 + 7]));
						}
					});

					mdl.start_group(fmt::format("Faces {}", geoset.faces.size()), [&]() {
						mdl.start_group("Triangles", [&]() { // Yall mfs gonna be having triangles, I ain't in the quad business
							std::string triangles;
							for (const auto& face : geoset.faces) {
								triangles += fmt::format("{}, ", face);
							}

							mdl.write_line(fmt::format("{{ {} }}", triangles));
						});
					});

					mdl.write_line(fmt::format("MinimumExtent {{ {}, {}, {} }},", geoset.extent.minimum.x, geoset.extent.minimum.z, geoset.extent.minimum.z));
					mdl.write_line(fmt::format("MaximumExtent {{ {}, {}, {} }},", geoset.extent.maximum.x, geoset.extent.maximum.z, geoset.extent.maximum.z));
					mdl.write_line(fmt::format("BoundsRadius {},", geoset.extent.bounds_radius));

					for (const auto& i : geoset.extents) {
						mdl.start_group("Anim", [&]() {
							mdl.write_line(fmt::format("MinimumExtent {{ {}, {}, {} }},", i.minimum.x, i.minimum.z, i.minimum.z));
							mdl.write_line(fmt::format("MaximumExtent {{ {}, {}, {} }},", i.maximum.x, i.maximum.z, i.maximum.z));
							mdl.write_line(fmt::format("BoundsRadius {},", i.bounds_radius));
						});
					}

					// mdl.start_group(fmt::format("Group {} {}", geoset.matrix_groups), [&]() {
					//	for (const auto& face : geoset.matrix_groups) {
					//		mdl.write_line(fmt::format("Matrices {{ {} }}", ));
					//	}
					// });

					mdl.write_line(fmt::format("MaterialID {},", geoset.material_id));
					mdl.write_line(fmt::format("SelectionGroup {},", geoset.selection_group));
					mdl.write_line("LevelOfDetail 0,");
					mdl.write_line(fmt::format("Name {},", geoset.lod_name));
				});
			}

			for (const auto& geoset_anim : animations) {
				mdl.start_group("GeosetAnim", [&]() {
					mdl.write_track(geoset_anim.KGAO, "Alpha", geoset_anim.alpha);
					mdl.write_track(geoset_anim.KGAC, "Color", geoset_anim.color);
					mdl.write_line(fmt::format("GeosetId {}", geoset_anim.geoset_id));
				});
			}

			for (const auto& bone : bones) {
				mdl.start_group(fmt::format("Bone \"{}\"", bone.node.name), [&]() {
					mdl.write_line(fmt::format("GeosetId {},", bone.geoset_id));			   // The MDL has "Multiple" as value for some reason
					mdl.write_line(fmt::format("GeosetAnimId {},", bone.geoset_animation_id)); // And this one has "None"

					mdl.write_node(bone.node);
				});
			}

			for (const auto& help_bone : help_bones) {
				mdl.start_group(fmt::format("Helper \"{}\"", help_bone.name), [&]() {
					mdl.write_node(help_bone);
				});
			}

			for (const auto& attachment : attachments) {
				mdl.start_group(fmt::format("Helper \"{}\"", attachment.node.name), [&]() {
					mdl.write_node(attachment.node);
					mdl.write_line(fmt::format("AttachmentID {},", attachment.attachment_id));
					mdl.write_track(attachment.KATV, "Visibility", 0.f); // dunno
				});
			}

			mdl.start_group(fmt::format("PivotPoints \"{}\"", pivots.size()), [&]() {
				for (const auto& pivot : pivots) {
					mdl.write_line(fmt::format("{{ {}, {}, {} }},", pivot.x, pivot.y, pivot.z));
				}
			});

			for (const auto& emitter : emitters2) {
				mdl.start_group(fmt::format("ParticleEmitter2 \"{}\"", emitter.node.name), [&]() {
					mdl.write_node(emitter.node);

					mdl.write_track(emitter.KP2S, "Speed", emitter.speed);
					mdl.write_track(emitter.KP2R, "Variation", emitter.variation);
					mdl.write_track(emitter.KP2L, "Latitude", emitter.latitude);
					mdl.write_track(emitter.KP2G, "Gravity", emitter.gravity);
					if (emitter.squirt) {
						mdl.write_line("Squirt,");
					}
					mdl.write_track(emitter.KP2V, "Visibility", 0.f); // ToDo static value
					mdl.write_line(fmt::format("Lifespan {},", emitter.life_span));
					mdl.write_track(emitter.KP2E, "EmissionRate", emitter.emission_rate);
					mdl.write_track(emitter.KP2W, "Width", emitter.width);
					mdl.write_track(emitter.KP2N, "Length", emitter.length);

					switch (emitter.filter_mode) {
						case 0:
							mdl.write_line("Blend");
							break;
						case 1:
							mdl.write_line("Additive");
							break;
						case 2:
							mdl.write_line("Modulate");
							break;
						case 3:
							mdl.write_line("Modulate2x");
							break;
						case 4:
							mdl.write_line("AlphaKey");
							break;
					}

					mdl.write_line(fmt::format("Rows {},", emitter.rows));
					mdl.write_line(fmt::format("Columns {},", emitter.columns));

					if (emitter.head_or_tail == 0) {
						mdl.write_line("Head,");
					} else if (emitter.head_or_tail == 0) {
						mdl.write_line("Tail,");
					} else {
						mdl.write_line("Both,");
					}

					mdl.write_line(fmt::format("TailLength {},", emitter.tail_length));
					mdl.write_line(fmt::format("Time {},", emitter.time_middle));

					mdl.start_group("SegmentColor", [&]() {
						mdl.write_line(fmt::format("Color {{ {}, {}, {}  }},", emitter.start_segment_color.x, emitter.start_segment_color.y, emitter.start_segment_color.z));
						mdl.write_line(fmt::format("Color {{ {}, {}, {}  }},", emitter.middle_segment_color.x, emitter.middle_segment_color.y, emitter.middle_segment_color.z));
						mdl.write_line(fmt::format("Color {{ {}, {}, {}  }},", emitter.end_segment_color.x, emitter.end_segment_color.y, emitter.end_segment_color.z));
					});

					mdl.write_line(fmt::format("Alpha {{ {}, {}, {}  }},", emitter.segment_alphas.x, emitter.segment_alphas.y, emitter.segment_alphas.z));
					mdl.write_line(fmt::format("ParticleScaling {{ {}, {}, {}  }},", emitter.segment_scaling.x, emitter.segment_scaling.y, emitter.segment_scaling.z));
					mdl.write_line(fmt::format("LifeSpanUVAnim {{ {}, {}, {}  }},", emitter.head_intervals.x, emitter.head_intervals.y, emitter.head_intervals.z));
					mdl.write_line(fmt::format("DecayUVAnim {{ {}, {}, {}  }},", emitter.head_decay_intervals.x, emitter.head_decay_intervals.y, emitter.head_decay_intervals.z));
					mdl.write_line(fmt::format("TailUVAnim {{ {}, {}, {}  }},", emitter.tail_intervals.x, emitter.tail_intervals.y, emitter.tail_intervals.z));
					mdl.write_line(fmt::format("TailDecayUVAnim {{ {}, {}, {}  }},", emitter.tail_decay_intervals.x, emitter.tail_decay_intervals.y, emitter.tail_decay_intervals.z));

					mdl.write_line(fmt::format("TextureID {},", emitter.texture_id));
					mdl.write_line(fmt::format("PriorityPlane {},", emitter.priority_plane));
				});
			}

			for (const auto& event_object : event_objects) {
				mdl.start_group(fmt::format("EventObject \"{}\"", event_object.node.name), [&]() {
					mdl.write_node(event_object.node);

					mdl.start_group(fmt::format("EventTrack {}", event_object.times.size()), [&]() {
						for (const auto& track : event_object.times) {
							mdl.write_line(fmt::format("{},", track));
						}
					});
				});
			}

			for (const auto& collision_shape : collision_shapes) {
				mdl.start_group(fmt::format("CollisionShape \"{}\"", collision_shape.node.name), [&]() {
					mdl.write_node(collision_shape.node);

					switch (collision_shape.type) {
						case CollisionShape::Shape::Box:
							mdl.write_line("Cube,");
							mdl.start_group("Vertices 2", [&]() {
								mdl.write_line(fmt::format("{{ {}, {}, {}  }},", collision_shape.vertices[0].x, collision_shape.vertices[0].y, collision_shape.vertices[0].z));
								mdl.write_line(fmt::format("{{ {}, {}, {}  }},", collision_shape.vertices[1].x, collision_shape.vertices[1].y, collision_shape.vertices[1].z));
							});
							break;
						case CollisionShape::Shape::Plane:
							mdl.write_line("Plane,");
							mdl.start_group("Vertices 2", [&]() {
								mdl.write_line(fmt::format("{{ {}, {}, {}  }},", collision_shape.vertices[0].x, collision_shape.vertices[0].y, collision_shape.vertices[0].z));
								mdl.write_line(fmt::format("{{ {}, {}, {}  }},", collision_shape.vertices[1].x, collision_shape.vertices[1].y, collision_shape.vertices[1].z));
							});
							break;
						case CollisionShape::Shape::Sphere:
							mdl.write_line("Sphere,");
							mdl.start_group("Vertices 1", [&]() {
								mdl.write_line(fmt::format("{{ {}, {}, {}  }},", collision_shape.vertices[0].x, collision_shape.vertices[0].y, collision_shape.vertices[0].z));
							});
							mdl.write_line(fmt::format("BoundsRadius {},", collision_shape.radius));
							break;
						case CollisionShape::Shape::Cylinder:
							mdl.write_line("Cylinder,");
							mdl.start_group("Vertices 2", [&]() {
								mdl.write_line(fmt::format("{{ {}, {}, {}  }},", collision_shape.vertices[0].x, collision_shape.vertices[0].y, collision_shape.vertices[0].z));
								mdl.write_line(fmt::format("{{ {}, {}, {}  }},", collision_shape.vertices[1].x, collision_shape.vertices[1].y, collision_shape.vertices[1].z));
							});
							mdl.write_line(fmt::format("BoundsRadius {},", collision_shape.radius));
							break;
					}
				});
			}
			return mdl.mdl;
		}

		static MDX from_mdl(std::string_view mdl);

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
		std::vector<EventObject> event_objects;
		std::vector<CollisionShape> collision_shapes;
		std::vector<CornEmitter> corn_emitters;
		std::vector<FaceFX> facefxes;

		std::vector<Camera> cameras;
		std::vector<float> bind_poses;
		std::vector<TextureAnimation> texture_animations;

		void forEachNode(const std::function<void(Node&)>& F) {
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
	};
} // namespace mdx