#include "MDX.h"

#include <iostream>
#include "Utilities.h"
#include "SkeletalModelInstance.h"

namespace mdx {
	std::map<int, std::string> replacable_id_to_texture{
		{ 1, "ReplaceableTextures/TeamColor/TeamColor00.dds" },
		{ 2, "ReplaceableTextures/TeamGlow/TeamGlow00.dds" },
		{ 11, "ReplaceableTextures/Cliff/Cliff0.dds" },
		{ 31, "ReplaceableTextures/LordaeronTree/LordaeronFallTree.dds" },
		{ 32, "ReplaceableTextures/AshenvaleTree/AshenTree.dds" },
		{ 33, "ReplaceableTextures/BarrensTree/BarrensTree.dds" },
		{ 34, "ReplaceableTextures/NorthrendTree/NorthTree.dds" },
		{ 35, "ReplaceableTextures/Mushroom/MushroomTree.dds" },
		{ 36, "ReplaceableTextures/RuinsTree/RuinsTree.dds" },
		{ 37, "ReplaceableTextures/OutlandMushroomTree/MushroomTree.dds" }
	};

	void AnimatedData::load_tracks(BinaryReader& reader) {
		TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());

		switch (tag) {
		case TrackTag::KMTF:
		case TrackTag::KLAS:
		case TrackTag::KLAE:
		case TrackTag::KRTX:
		case TrackTag::KCRL:
			tracks.emplace(tag, TrackHeader<uint32_t>(reader));
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
		case TrackTag::KFCA:
		case TrackTag::KFTC:
		case TrackTag::KMTE:
			tracks.emplace(tag, TrackHeader<float>(reader));
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
		case TrackTag::KFC3:
			tracks.emplace(tag, TrackHeader<glm::vec3>(reader));
			break;
		case TrackTag::KTAR:
		case TrackTag::KGRT:
			tracks.emplace(tag, TrackHeader<glm::quat>(reader));
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

	Texture::Texture(BinaryReader& reader) {
		replaceable_id = reader.read<uint32_t>();
		file_name = reader.read_string(260);
		flags = reader.read<uint32_t>();
	}

	Node::Node(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t inclusive_size = reader.read<uint32_t>();
		name = reader.read_string(80);
		id = reader.read<uint32_t>();
		parent_id = reader.read<uint32_t>();
		flags = reader.read<uint32_t>();

		while (reader.position < reader_pos + inclusive_size) {
			animated_data.load_tracks(reader);
		}
	}

	float Node::getVisibility(SkeletalModelInstance& instance) const {
		return 1.0f;
	}

	float Layer::getVisibility(int frame, const SkeletalModelInstance& instance) const {
		if (animated_data.has_track(TrackTag::KMTA)) {
			return animated_data.track<float>(TrackTag::KMTA).matrixEaterInterpolate(frame, instance, alpha);
		} else {
			return alpha;
		}
	}

	glm::vec3 GeosetAnimation::getColor(int frame, SkeletalModelInstance& instance) const {
		if (animated_data.has_track(TrackTag::KGAC)) {
			return animated_data.track<glm::vec3>(TrackTag::KGAC).matrixEaterInterpolate(frame, instance, color);
		} else {
			return color;
		}
	}

	float GeosetAnimation::getVisibility(int frame, SkeletalModelInstance& instance) const {
		if (animated_data.has_track(TrackTag::KGAO)) {
			return animated_data.track<float>(TrackTag::KGAO).matrixEaterInterpolate(frame, instance, alpha);
		} else {
			return alpha;
		}
	}

	template <typename T>
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
		} else if (ceilAfterEnd || (ceilIndexTime < time && tracks[floorAnimEndIndex].frame < time)) {
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

	template <typename T>
	T Node::getValue(mdx::TrackTag tag, SkeletalModelInstance& instance, const T& defaultValue) const {
		if (animated_data.has_track(tag)) {
			return animated_data.track<T>(tag).matrixEaterInterpolate(instance.current_frame, instance, defaultValue);
		} else {
			return defaultValue;
		}
	}
	template glm::vec3 Node::getValue(mdx::TrackTag tag, SkeletalModelInstance& instance, const glm::vec3& defaultValue) const;
	template glm::quat Node::getValue(mdx::TrackTag tag, SkeletalModelInstance& instance, const glm::quat& defaultValue) const;

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
			case ChunkTag::VERS:
				reader.advance(4);
				version = reader.read<uint32_t>();
				break;
			case ChunkTag::SEQS:
				read_SEQS_chunk(reader);
				break;
			case ChunkTag::GLBS:
				read_GLBS_chunk(reader);
				break;
			case ChunkTag::MTLS:
				read_MTLS_chunk(reader);
				break;
			case ChunkTag::TEXS:
				read_TEXS_chunk(reader);
				break;
			case ChunkTag::GEOS:
				read_GEOS_chunk(reader);
				break;
			case ChunkTag::GEOA:
				read_GEOA_chunk(reader);
				break;
			case ChunkTag::BONE:
				read_BONE_chunk(reader);
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
			default:
				reader.advance(reader.read<uint32_t>());
			}
		}

		// Remove geoset animations that reference non existing geosets
		for (size_t i = animations.size(); i-- > 0;) {
			if (animations[i].geoset_id >= geosets.size()) {
				animations.erase(animations.begin() + i);
			}
		}

		// Standardize animation names
		// Not sure how animation names work exactly so this is just an easy way to get a valid stand animation so we can do geoset hiding
		//int t = 0;
		//for (auto& i : sequences) {
		//	std::transform(i.name.begin(), i.name.end(), i.name.begin(), ::tolower);

		//	auto parts = split(i.name, ' ');
		//	i.name = parts.front() + std::to_string(t++);
		//}
	}

	void MDX::read_GEOS_chunk(BinaryReader& reader) {
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
			geoset.bone_groups = reader.read_vector<uint32_t>(matrix_group_count);
			reader.advance(4); // Mats
			const uint32_t matrix_indices_count = reader.read<uint32_t>();
			geoset.bone_indices = reader.read_vector<uint32_t>(matrix_indices_count);
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

	void MDX::read_MTLS_chunk(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		uint32_t total_size = 0;

		while (total_size < size) {
			total_size += reader.read<uint32_t>();

			Material material;
			material.priority_plane = reader.read<uint32_t>();
			material.flags = reader.read<uint32_t>();
			if (version > 800) {
				reader.advance(80); // A shader file
			}
			reader.advance(4);
			const uint32_t layers_count = reader.read<uint32_t>();

			for (size_t i = 0; i < layers_count; i++) {
				const int reader_pos = reader.position;
				Layer layer;
				const uint32_t size = reader.read<uint32_t>();
				layer.blend_mode = reader.read<uint32_t>();
				layer.shading_flags = reader.read<uint32_t>();
				layer.texture_id = reader.read<uint32_t>();
				layer.texture_animation_id = reader.read<uint32_t>();
				layer.coord_id = reader.read<uint32_t>();
				layer.alpha = reader.read<float>();

				if (version > 800) {
					reader.advance(4);	// emissiveGain
					reader.advance(12); // fresnelColor
					reader.advance(4);	// fresnelOpacity
					reader.advance(4);	// fresnelTeamColor
				}

				while (reader.position < reader_pos + size) {
					layer.animated_data.load_tracks(reader);
				}

				material.layers.push_back(std::move(layer));
			}

			materials.push_back(std::move(material));
		}
	}

	void MDX::read_SEQS_chunk(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		for (size_t i = 0; i < size / 132; i++) {
			Sequence sequence;
			sequence.name = reader.read_string(80);
			sequence.interval_start = reader.read<uint32_t>();
			sequence.interval_end = reader.read<uint32_t>();
			sequence.movespeed = reader.read<float>();
			sequence.flags = reader.read<uint32_t>();
			sequence.rarity = reader.read<float>();
			sequence.sync_point = reader.read<uint32_t>();
			sequence.extent = Extent(reader);
			sequences.push_back(std::move(sequence));
		}
	}

	void MDX::read_GEOA_chunk(BinaryReader& reader) {
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
				animation.animated_data.load_tracks(reader);
			}

			animations.push_back(std::move(animation));
		}
	}

	void MDX::read_BONE_chunk(BinaryReader& reader) {
		const size_t reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Bone bone;
			bone.node = Node(reader);
			bone.geoset_id = reader.read<int32_t>();
			bone.geoset_animation_id = reader.read<int32_t>();
			bones.push_back(std::move(bone));
		}
	}

	void MDX::read_TEXS_chunk(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		for (size_t i = 0; i < size / 268; i++) {
			textures.emplace_back(Texture(reader));
		}
	}

	void MDX::read_GLBS_chunk(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		global_sequences = reader.read_vector<uint32_t>(size / 4);
	}

	void MDX::read_LITE_chunk(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Light light;
			const size_t node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			light.node = Node(reader);
			light.type = reader.read<uint32_t>();
			light.attenuation_start = reader.read<float>();
			light.attenuation_end = reader.read<float>();
			light.color = reader.read<glm::vec3>();
			light.intensity = reader.read<float>();
			light.ambient_color = reader.read<glm::vec3>();
			light.ambient_intensity = reader.read<float>();
			while (reader.position < node_reader_pos + inclusive_size) {
				light.node.animated_data.load_tracks(reader);
			}
			lights.push_back(std::move(light));
		}
	}

	void MDX::read_HELP_chunk(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();
		while (reader.position < reader_pos + size) {
			help_bones.push_back(Node(reader));
		}
	}

	void MDX::read_ATCH_chunk(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Attachment attachment;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			attachment.node = Node(reader);
			attachment.path = reader.read_string(256);
			attachment.reserved = reader.read<uint32_t>();
			attachment.attachment_id = reader.read<uint32_t>();
			while (reader.position < node_reader_pos + inclusive_size) {
				attachment.node.animated_data.load_tracks(reader);
			}
			attachments.push_back(std::move(attachment));
		}
	}

	void MDX::read_PIVT_chunk(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		pivots = reader.read_vector<glm::vec3>(size / 12);
	}

	void MDX::read_PREM_chunk(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			ParticleEmitter1 emitter;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter.node = Node(reader);
			emitter.emission_rate = reader.read<float>();
			emitter.gravity = reader.read<float>();
			emitter.longitude = reader.read<float>();
			emitter.latitude = reader.read<float>();
			emitter.path = reader.read_string(256);
			emitter.reserved = reader.read<uint32_t>();
			emitter.life_span = reader.read<float>();
			emitter.speed = reader.read<float>();
			while (reader.position < node_reader_pos + inclusive_size) {
				emitter.node.animated_data.load_tracks(reader);
			}
			emitters1.push_back(std::move(emitter));
		}
	}

	void MDX::read_PRE2_chunk(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			ParticleEmitter2 emitter2;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter2.node = Node(reader);

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
			for (int time = 0; time < 3; time++) {
				for (int i = 0; i < 3; i++) {
					emitter2.segment_color[time][i] = reader.read<float>();
				}
			}
			for (int i = 0; i < 3; i++) {
				emitter2.segment_alphas[i] = reader.read<uint8_t>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.segment_scaling[i] = reader.read<float>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.head_intervals[i] = reader.read<float>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.head_decay_intervals[i] = reader.read<float>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.tail_intervals[i] = reader.read<float>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.tail_decay_intervals[i] = reader.read<float>();
			}
			emitter2.texture_id = reader.read<uint32_t>();
			emitter2.squirt = reader.read<uint32_t>();
			emitter2.priority_plane = reader.read<uint32_t>();
			emitter2.replaceable_id = reader.read<uint32_t>();

			while (reader.position < node_reader_pos + inclusive_size) {
				emitter2.node.animated_data.load_tracks(reader);
			}
			emitters2.push_back(std::move(emitter2));
		}
	}

	void MDX::read_RIBB_chunk(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			RibbonEmitter emitter;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter.node = Node(reader);
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
				emitter.node.animated_data.load_tracks(reader);
			}
			ribbons.push_back(std::move(emitter));
		}
	}

	void MDX::read_EVTS_chunk(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			EventObject evt;
			evt.node = Node(reader);
			reader.read<uint32_t>(); // read KEVT
			evt.count = reader.read<uint32_t>();
			evt.global_sequence_id = reader.read<int32_t>(); //signed
			for (int i = 0; i < evt.count; i++) {
				evt.times.push_back(reader.read<uint32_t>());
			}
			eventObjects.push_back(std::move(evt));
		}
	}

	void MDX::read_CLID_chunk(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			CollisionShape shape;
			shape.node = Node(reader);

			uint32_t type_index = reader.read<uint32_t>();
			switch (type_index) {
			case 1:
				shape.type = CollisionShapeType::Plane;
				break;
			case 2:
				shape.type = CollisionShapeType::Sphere;
				break;
			case 3:
				shape.type = CollisionShapeType::Cylinder;
				break;
			default:
			case 0:
				shape.type = CollisionShapeType::Box;
				break;
			}

			for (int i = 0; i < 3; i++) {
				if (reader.remaining() <= 0) {
					shape.vertices[0][i] = 0;
				} else {
					shape.vertices[0][i] = reader.read<float>();
				}
			}

			if (type_index != 2) {
				for (int i = 0; i < 3; i++) {
					if (reader.remaining() <= 0) {
						shape.vertices[1][i] = 0;
					} else {
						shape.vertices[1][i] = reader.read<float>();
					}
				}
			}

			if (type_index == 2 || type_index == 3) {
				if (reader.remaining() > 0) {
					shape.radius = reader.read<float>();
				} else {
					shape.radius = 0;
				}
			}
			collisionShapes.push_back(std::move(shape));
		}
	}

	void MDX::read_CORN_chunk(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			CornEmitter emitter;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter.node = Node(reader);
			
			reader.advance(4);
			reader.advance(4);
			reader.advance(4);
			reader.advance(16);
			reader.advance(4);
			reader.advance(260);
			reader.advance(260);

			reader.advance(inclusive_size - (reader.position - node_reader_pos));

			//while (reader.position < node_reader_pos + inclusive_size) {
			//	emitter.node.animated_data.load_tracks(reader);
			//}
			corn_emitters.push_back(std::move(emitter));
		}
	}


	void MDX::forEachNode(const std::function<void(Node&)>& F) {
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

		for (auto& i : eventObjects) {
			F(i.node);
		}

		for (auto& i : collisionShapes) {
			F(i.node);
		}
		for (auto& i : corn_emitters) {
			F(i.node);
		}
	}
} // namespace mdx