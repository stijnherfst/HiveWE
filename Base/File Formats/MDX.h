#pragma once

#include <map>
#include <unordered_map>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "BinaryReader.h"

#include <filesystem>
#include <variant>
#include <functional>

namespace fs = std::filesystem;

class SkeletalModelInstance;
struct Sequence;

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
		KGSC = 0x4353474b,
		KFC3 = 860046923,
		KFCA = 1094927947,
		KFTC = 1129596491,
		KMTE = 1163152715

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
		LITE = 1163151692,
		HELP = 1347175752,
		ATCH = 1212372033,
		PIVT = 1414941008,
		PREM = 1296388688,
		PRE2 = 843403856,
		RIBB = 1111640402,
		EVTS = 1398036037,
		CLID = 1145654339,
		GLBS = 1396853831
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
		int32_t interpolation_type;
		int32_t global_sequence_ID;
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

		// We have to pass the sequence here because
		// the return value is dependent on start/end time
		template <typename T>
		int ceilIndex(int frame, mdx::Sequence& sequence) const {
			// ToDo fix, this function does not work yet.
			int trackSize = tracks.size();
			if (frame < sequence.interval_start) {
				return -1;
			} else if (frame >= sequence.interval_end) {
				return trackSize;
			} else {
				for (int i = 1; i < trackSize; i++) {
					const Track<T>& track = tracks[i];
					if (track.frame > frame) {
						return i;
					}
				}
				return -1;
			}
			return 0;
		}

		template <typename T>
		void getValue(T& out, int frame, mdx::Sequence& sequence, const T& defaultValue) const {
			// ToDo fix, this function does not work yet.
			int index = ceilIndex(frame, sequence);
			int length = tracks.size();
			if (index == -1) {
				out = tracks[0].value;
			} else if (index == length) {
				out = tracks[length - 1].value;
			} else {
				Track<T>& start = tracks[index - 1];
				Track<T>& end = tracks[index];
				float t = clampValue((frame - start.frame) / (end.frame - start.frame), 0, 1);
			}
		}

		/* Matrix Eater interpolate is slower than ghostwolf interpolate,
   because this one does more CPU operations. But it does not
   require modifying the model on load to inject additional keyframes
	*/

		T TrackHeader<T>::matrixEaterInterpolate(int time, const SkeletalModelInstance& instance, const T& defaultValue) const {
			int sequenceStart;
			int sequenceEnd;
			if (global_sequence_ID >= 0 && instance.model->global_sequences.size()) {
				sequenceStart = 0;
				sequenceEnd = instance.model->global_sequences[global_sequence_ID];
				if (sequenceEnd == 0) {
					time = 0;
				} else {
					time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % sequenceEnd;
				}
			} else if (instance.model->sequences.size() && instance.sequence_index != -1) {
				Sequence& sequence = instance.model->sequences[instance.sequence_index];
				sequenceStart = sequence.interval_start;
				sequenceEnd = sequence.interval_end;
			} else {
				return defaultValue;
			}
			if (tracks.empty()) {
				return defaultValue;
			}
			int ceilIndex = -1;
			int floorIndex = 0;
			const T* floorInTan;
			const T* floorOutTan = 0;
			const T* floorValue;
			const T* ceilValue;
			int floorIndexTime;
			int ceilIndexTime;
			// ToDo "if global seq" check is here in MXE java

			int floorAnimStartIndex = 0;
			int floorAnimEndIndex = 0;
			// get floor:
			int tracksSize = tracks.size();
			for (int i = 0; i < tracksSize; i++) {
				const Track<T>& track = tracks[i];
				if (track.frame <= sequenceStart) {
					floorAnimStartIndex = i;
				}
				if (track.frame <= time) {
					floorIndex = i;
				}
				if (track.frame >= time && ceilIndex == -1) {
					ceilIndex = i;
				}
				if (track.frame <= sequenceEnd) {
					floorAnimEndIndex = i;
				} else {
					// end of our sequence
					break;
				}
			}
			if (ceilIndex == -1) {
				ceilIndex = tracksSize - 1;
			}
			// end get floor
			if (ceilIndex < floorIndex) {
				ceilIndex = floorIndex;
				// was a problem in matrix eater, different impl, not problem here?
			}
			floorValue = &tracks[floorIndex].value;
			if (interpolation_type > 1) {
				floorInTan = &tracks[floorIndex].inTan;
				floorOutTan = &tracks[floorIndex].outTan;
			}
			ceilValue = &tracks[ceilIndex].value;
			floorIndexTime = tracks[floorIndex].frame;
			ceilIndexTime = tracks[ceilIndex].frame;
			if (ceilIndexTime < sequenceStart) {
				return defaultValue;
			}
			if (floorIndexTime > sequenceEnd) {
				return defaultValue;
			}
			auto floorBeforeStart = floorIndexTime < sequenceStart;
			auto ceilAfterEnd = ceilIndexTime > sequenceEnd;
			if (floorBeforeStart && ceilAfterEnd) {
				return defaultValue;
			} else if (floorBeforeStart) {
				if (tracks[floorAnimEndIndex].frame == sequenceEnd) {
					// no "floor" frame found, but we have a ceil frame,
					// so the prev frame is a repeat of animation's end
					// placed at the beginning
					floorIndex = floorAnimEndIndex;
					floorValue = &tracks[floorAnimEndIndex].value;
					floorIndexTime = sequenceStart;
					if (interpolation_type > 1) {
						floorInTan = &tracks[floorAnimEndIndex].inTan;
						floorOutTan = &tracks[floorAnimEndIndex].outTan;
					}
				} else {
					floorValue = &defaultValue;
					floorInTan = floorOutTan = &defaultValue;
					floorIndexTime = sequenceStart;
				}
			} else if (ceilAfterEnd || ceilIndexTime < time && tracks[floorAnimEndIndex].frame < time) {
				// if we have a floor frame but the "ceil" frame is after end of sequence,
				// or our ceil frame is before our time, meaning that we're at the end of the
				// entire timeline, then we need to inject a "ceil" frame at end of sequence
				if (tracks[floorAnimStartIndex].frame == sequenceStart) {
					ceilValue = &tracks[floorAnimStartIndex].value;
					ceilIndex = floorAnimStartIndex;
					ceilIndexTime = sequenceStart;
				}
				// for the else case here, Matrix Eater code says to leave it blank,
				// example model is Water Elemental's birth animation, to verify behavior
			}
			if (floorIndex == ceilIndex) {
				return *floorValue;
			}
			const T* ceilInTan = &tracks[ceilIndex].inTan;
			float t = std::clamp((time - floorIndexTime) / (float)(ceilIndexTime - floorIndexTime), 0.f, 1.f);
		
			return interpolate(floorValue, floorOutTan, ceilInTan, ceilValue, t, interpolation_type);
		}
	};

	struct AnimatedData {
		std::unordered_map<TrackTag, std::variant<TrackHeader<float>,
												  TrackHeader<uint32_t>,
												  TrackHeader<glm::vec3>,
												  TrackHeader<glm::quat>>>
			tracks;

		void load_tracks(BinaryReader& reader);

		template <typename T>
		const TrackHeader<T>& track(const TrackTag track) const {
			return std::get<TrackHeader<T>>(tracks.at(track));
		}

		bool has_track(const TrackTag track) const {
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

	struct Layer {
		uint32_t blend_mode;
		uint32_t shading_flags;
		uint32_t texture_id;
		uint32_t texture_animation_id;
		uint32_t coord_id;
		float alpha;

		AnimatedData animated_data;

		float getVisibility(int frame, const SkeletalModelInstance& instance) const;
	};

	struct Node {
		Node() = default;
		explicit Node(BinaryReader& reader);

		std::string name;
		int id;
		int parent_id;
		int flags;
		AnimatedData animated_data;

		float getVisibility(SkeletalModelInstance& instance) const;

		template <typename T>
		T getValue(mdx::TrackTag tag, SkeletalModelInstance& instance, const T& defaultValue) const {
			if (animated_data.has_track(tag)) {
				return animated_data.track<T>(tag).matrixEaterInterpolate(instance.current_frame, instance, defaultValue);
			} else {
				return defaultValue;
			}
		}

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

	struct Sequence {
		std::string name;
		uint32_t interval_start;
		uint32_t interval_end;
		float movespeed;
		uint32_t flags;
		float rarity;
		uint32_t sync_point;
		Extent extent;
	};

	struct Geoset {
		uint32_t lod;
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<uint32_t> face_type_groups;
		std::vector<uint32_t> face_groups;
		std::vector<uint16_t> faces;
		std::vector<uint8_t> vertex_groups;
		std::vector<uint32_t> matrix_groups;
		std::vector<uint32_t> node_indices;

		uint32_t material_id;
		uint32_t selection_group;
		uint32_t selection_flags;
		Extent extent;

		std::vector<Extent> extents;

		using TextureCoordinateSet = std::vector<glm::vec2>;
		std::vector<TextureCoordinateSet> texture_coordinate_sets;
	};

	struct GeosetAnimation {
		float alpha;
		uint32_t flags;
		glm::vec3 color;
		uint32_t geoset_id;
		AnimatedData animated_data;

		glm::vec3 getColor(int frame, SkeletalModelInstance& instance) const;
		float getVisibility(int frame, SkeletalModelInstance& instance) const;
	};

	struct Texture {
		explicit Texture(BinaryReader& reader);
		uint32_t replaceable_id;
		fs::path file_name;
		uint32_t flags;
	};

	struct Material {
		uint32_t priority_plane;
		uint32_t flags;
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
	};

	struct Attachment {
		Node node;
		std::string path; // Reference to Undead, NE, or Naga birth anim
		int reserved;	  // ToDo mine meaning of reserved from Game.dll, likely strlen
		int attachment_id;
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
		float segment_color[3][3]; // rows [Begin, Middle, End], column is color
		uint8_t segment_alphas[3];
		float segment_scaling[3];
		uint32_t head_intervals[3];
		uint32_t head_decay_intervals[3];
		uint32_t tail_intervals[3];
		uint32_t tail_decay_intervals[3];
		uint32_t texture_id;
		uint32_t squirt;
		uint32_t priority_plane;
		uint32_t replaceable_id; // for Wisp team color particles
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
		uint32_t count;
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
		float vertices[2][3]; // sometimes only 1 is used
		float radius;		  // used for sphere/cylinder
	};

	class MDX {
		int version;

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

	  public:
		explicit MDX(BinaryReader& reader);
		void load(BinaryReader& reader);

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

		void forEachNode(const std::function<void(Node&)>& lambda);
	};
} // namespace mdx