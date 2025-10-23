module MDX;

import std;
import <glm/glm.hpp>;

namespace fs = std::filesystem;

namespace mdx {
	void write_GEOS(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.geosets.empty()) {
			return;
		}

		writer.write(ChunkTag::GEOS);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& geoset : mdx.geosets) {
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
			writer.write<uint32_t>(geoset.sequence_extents.size());
			for (const auto& extent : geoset.sequence_extents) {
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
			writer.write<uint32_t>(geoset.uv_sets.size());
			for (const auto& set : geoset.uv_sets) {
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

	void write_MTLS(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.materials.empty()) {
			return;
		}

		writer.write(ChunkTag::MTLS);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& material : mdx.materials) {
			// Write temporary zero, remember location
			const size_t material_index = writer.buffer.size();
			writer.write<uint32_t>(0);

			writer.write<uint32_t>(material.priority_plane);
			writer.write<uint32_t>(material.flags);
			writer.write_string("LAYS");
			writer.write<uint32_t>(material.layers.size());

			for (const auto& layer : material.layers) {
				// Write temporary zero, remember location
				const size_t layer_index = writer.buffer.size();
				writer.write<uint32_t>(0);

				writer.write<uint32_t>(layer.blend_mode);
				writer.write<uint32_t>(layer.shading_flags);
				writer.write<uint32_t>(0); // texture_id irrelevant when writing V1100
				writer.write<uint32_t>(layer.texture_animation_id);
				writer.write<uint32_t>(layer.coord_id);
				writer.write<float>(layer.alpha);

				writer.write<float>(layer.emissive_gain);
				writer.write<glm::vec3>(layer.fresnel_color);
				writer.write<float>(layer.fresnel_opacity);
				writer.write<float>(layer.fresnel_team_color);

				writer.write<uint32_t>(layer.hd);
				writer.write<uint32_t>(layer.textures.size());

				for (size_t i = 0; i < layer.textures.size(); i++) {
					writer.write<uint32_t>(layer.textures[i].id);
					writer.write<uint32_t>(i);
					layer.textures[i].KMTF.save(TrackTag::KMTF, writer);
				}

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

	void write_SEQS(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.sequences.empty()) {
			return;
		}

		writer.write(ChunkTag::SEQS);
		writer.write<uint32_t>(mdx.sequences.size() * 132);
		for (const auto& i : mdx.sequences) {
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

	void write_GLBS(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.global_sequences.empty()) {
			return;
		}

		writer.write(ChunkTag::GLBS);
		writer.write<uint32_t>(mdx.global_sequences.size() * 4);
		writer.write_vector(mdx.global_sequences);
	}

	void write_GEOA(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.animations.empty()) {
			return;
		}

		writer.write(ChunkTag::GEOA);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& geoset_animation : mdx.animations) {
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

	void write_BONE(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.bones.empty()) {
			return;
		}

		writer.write(ChunkTag::BONE);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& bone : mdx.bones) {
			bone.node.save(writer);
			writer.write<int32_t>(bone.geoset_id);
			writer.write<int32_t>(bone.geoset_animation_id);
		}
		const uint32_t temporary = writer.buffer.size() - inclusive_index - 4;
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void write_TEXS(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.textures.empty()) {
			return;
		}

		writer.write(ChunkTag::TEXS);
		writer.write<uint32_t>(mdx.textures.size() * 268);
		for (const auto& texture : mdx.textures) {
			writer.write<uint32_t>(texture.replaceable_id);
			writer.write_c_string_padded(texture.file_name.string(), 260);
			writer.write<uint32_t>(texture.flags);
		}
	}

	void write_LITE(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.lights.empty()) {
			return;
		}

		writer.write(ChunkTag::LITE);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& light : mdx.lights) {
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
			writer.write<float>(light.shadow_intensity);

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

	void write_HELP(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.help_bones.empty()) {
			return;
		}

		writer.write(ChunkTag::HELP);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& help_bone : mdx.help_bones) {
			help_bone.save(writer);
		}
		const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void write_ATCH(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.attachments.empty()) {
			return;
		}

		writer.write(ChunkTag::ATCH);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& attachment : mdx.attachments) {
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

	void write_PIVT(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.pivots.empty()) {
			return;
		}

		writer.write(ChunkTag::PIVT);
		writer.write<uint32_t>(mdx.pivots.size() * 12);
		for (const auto& pivot : mdx.pivots) {
			writer.write<glm::vec3>(pivot);
		}
	}

	void write_PREM(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.emitters1.empty()) {
			return;
		}

		writer.write(ChunkTag::PREM);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& emitter : mdx.emitters1) {
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

	void write_PRE2(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.emitters2.empty()) {
			return;
		}

		writer.write(ChunkTag::PRE2);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& emitter : mdx.emitters2) {
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

	void write_RIBB(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.ribbons.empty()) {
			return;
		}

		writer.write(ChunkTag::RIBB);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& ribbon : mdx.ribbons) {
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

	void write_EVTS(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.event_objects.empty()) {
			return;
		}

		writer.write(ChunkTag::EVTS);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& event_object : mdx.event_objects) {
			event_object.node.save(writer);
			writer.write_string("KEVT");
			writer.write<uint32_t>(event_object.times.size());
			writer.write<int32_t>(event_object.global_sequence_id);
			writer.write_vector(event_object.times);
		}
		const uint32_t temporary = static_cast<uint32_t>(writer.buffer.size() - inclusive_index - 4);
		std::memcpy(writer.buffer.data() + inclusive_index, &temporary, 4);
	}

	void write_CLID(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.collision_shapes.empty()) {
			return;
		}

		writer.write(ChunkTag::CLID);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& shape : mdx.collision_shapes) {
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

	void write_CORN(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.corn_emitters.empty()) {
			return;
		}

		writer.write(ChunkTag::CORN);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& corn : mdx.corn_emitters) {
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

	void write_CAMS(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.cameras.empty()) {
			return;
		}

		writer.write(ChunkTag::CAMS);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& camera : mdx.cameras) {
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

	void write_BPOS(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.bind_poses.empty()) {
			return;
		}

		writer.write(ChunkTag::BPOS);
		writer.write<uint32_t>(4 + mdx.bind_poses.size() * 4);
		writer.write<uint32_t>(mdx.bind_poses.size() / 12);
		writer.write_vector(mdx.bind_poses);
	}

	void write_TXAN(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.texture_animations.empty()) {
			return;
		}

		writer.write(ChunkTag::TXAN);
		// Write temporary zero, remember location
		const size_t inclusive_index = writer.buffer.size();
		writer.write<uint32_t>(0);

		for (const auto& texture_animation : mdx.texture_animations) {
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

	void write_FAFX(BinaryWriter& writer, const MDX& mdx) {
		if (mdx.facefxes.empty()) {
			return;
		}

		writer.write(ChunkTag::FAFX);
		writer.write<uint32_t>(mdx.facefxes.size() * 340);
		for (const auto& facefx : mdx.facefxes) {
			writer.write_c_string_padded(facefx.name, 80);
			writer.write_c_string_padded(facefx.path.string(), 260);
		}
	}

	BinaryWriter MDX::save() const {
		BinaryWriter writer;
		writer.write_string("MDLX");
		writer.write(ChunkTag::VERS);
		writer.write<uint32_t>(4);
		writer.write<uint32_t>(LATEST_MDX_VERSION);

		writer.write(ChunkTag::MODL);
		writer.write<uint32_t>(372);
		writer.write_c_string_padded(name, 80);
		writer.write_c_string_padded(animation_filename, 260);
		extent.save(writer);
		writer.write<uint32_t>(blend_time);

		write_SEQS(writer, *this);
		write_MTLS(writer, *this);
		write_TEXS(writer, *this);
		write_GEOS(writer, *this);
		write_GEOA(writer, *this);
		write_BONE(writer, *this);
		write_GLBS(writer, *this);
		write_LITE(writer, *this);
		write_HELP(writer, *this);
		write_ATCH(writer, *this);
		write_PIVT(writer, *this);
		write_PREM(writer, *this);
		write_PRE2(writer, *this);
		write_RIBB(writer, *this);
		write_CAMS(writer, *this);
		write_EVTS(writer, *this);
		write_CLID(writer, *this);
		write_CORN(writer, *this);
		write_FAFX(writer, *this);
		write_BPOS(writer, *this);
		write_TXAN(writer, *this);

		return writer;
	}
}