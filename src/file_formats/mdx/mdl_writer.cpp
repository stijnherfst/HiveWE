module MDX;

import std;
import Timer;
import <glm/glm.hpp>;

namespace mdx {
	/// A minimal utility wrapper around an std::string that manages newlines, indentation and closing braces
	struct MDLWriter {
		std::string mdl;
		size_t current_indentation = 0;

		template<typename... Args>
		void write_line(std::format_string<Args...> fmt, Args&&... args) {
			for (size_t i = 0; i < current_indentation; i++) {
				mdl += '\t';
			}
			mdl += std::vformat(fmt.get(), std::make_format_args(args...));
			mdl += '\n';
		}

		template<typename T>
		void write_track_body(const TrackHeader<T>& track_header, const std::string_view name) {
			start_group(std::format("{} {}", name, track_header.tracks.size()), [&]() {
				switch (track_header.interpolation_type) {
					case InterpolationType::none:
						write_line("DontInterp,");
						break;
					case InterpolationType::linear:
						write_line("Linear,");
						break;
					case InterpolationType::hermite:
						write_line("Hermite,");
						break;
					case InterpolationType::bezier:
						write_line("Bezier,");
						break;
				}

				if (track_header.global_sequence_ID != -1) {
					write_line("GlobalSeqId {},", track_header.global_sequence_ID);
				}

				for (const auto& track : track_header.tracks) {
					if constexpr (std::is_same_v<T, glm::vec2>) {
						write_line("{}: {{ {}, {} }},", track.frame, track.value.x, track.value.y);
					} else if constexpr (std::is_same_v<T, glm::vec3>) {
						write_line("{}: {{ {}, {}, {} }},", track.frame, track.value.x, track.value.y, track.value.z);
					} else if constexpr (std::is_same_v<T, glm::quat>) {
						write_line("{}: {{ {}, {}, {}, {} }},", track.frame, track.value.x, track.value.y, track.value.z, track.value.w);
					} else {
						write_line("{}: {},", track.frame, track.value);
					}

					if (track_header.interpolation_type == InterpolationType::hermite
						|| track_header.interpolation_type == InterpolationType::bezier) {
						if constexpr (std::is_same_v<T, glm::vec2>) {
							write_line("InTan {{ {}, {} }},", track.inTan.x, track.inTan.y);
							write_line("OutTan {{ {}, {} }},", track.outTan.x, track.outTan.y);
						} else if constexpr (std::is_same_v<T, glm::vec3>) {
							write_line("InTan {{ {}, {}, {} }},", track.inTan.x, track.inTan.y, track.inTan.z);
							write_line("OutTan {{ {}, {}, {} }},", track.outTan.x, track.outTan.y, track.outTan.z);
						} else if constexpr (std::is_same_v<T, glm::quat>) {
							write_line("InTan {{ {}, {}, {}, {} }},", track.inTan.x, track.inTan.y, track.inTan.z, track.inTan.w);
							write_line("OutTan {{ {}, {}, {}, {} }},", track.outTan.x, track.outTan.y, track.outTan.z, track.outTan.w);
						} else {
							write_line("InTan {},", track.inTan);
							write_line("OutTan {},", track.outTan);
						}
					}
				}
			});
		}

		/// Emit a track that has a static fallback (used for emitter fields, material layer fields).
		template<typename T>
		void write_track(const TrackHeader<T>& track_header, const std::string_view name, const T static_value) {
			if (track_header.tracks.empty()) {
				if constexpr (std::is_same_v<T, glm::vec2>) {
					write_line("static {} {{ {}, {} }},", name, static_value.x, static_value.y);
				} else if constexpr (std::is_same_v<T, glm::vec3>) {
					write_line("static {} {{ {}, {}, {} }},", name, static_value.x, static_value.y, static_value.z);
				} else if constexpr (std::is_same_v<T, glm::quat>) {
					write_line("static {} {{ {}, {}, {}, {} }},", name, static_value.x, static_value.y, static_value.z, static_value.w);
				} else {
					write_line("static {} {},", name, static_value);
				}
			} else {
				write_track_body(track_header, name);
			}
		}

		/// KGTR/KGRT/KGSC don't use statics.
		template<typename T>
		void write_node_track(const TrackHeader<T>& track_header, std::string name) {
			if (track_header.tracks.empty()) {
				return;
			}
			write_track_body(track_header, name);
		}

		void write_node(const Node& node, const bool is_particle_emitter_1 = false) {
			write_line("ObjectId {},", node.id);
			if (node.parent_id != -1) {
				write_line("Parent {},", node.parent_id);
			}

			if (node.flags & Node::Flags::dont_inherit_translation) {
				write_line("DontInherit {{ Translation }},");
			}
			if (node.flags & Node::Flags::dont_inherit_rotation) {
				write_line("DontInherit {{ Rotation }},");
			}
			if (node.flags & Node::Flags::dont_inherit_scaling) {
				write_line("DontInherit {{ Scaling }},");
			}
			if (node.flags & Node::Flags::billboarded) {
				write_line("Billboarded,");
			}
			if (node.flags & Node::Flags::billboarded_lock_x) {
				write_line("BillboardedLockX,");
			}
			if (node.flags & Node::Flags::billboarded_lock_y) {
				write_line("BillboardedLockY,");
			}
			if (node.flags & Node::Flags::billboarded_lock_z) {
				write_line("BillboardedLockZ,");
			}
			if (node.flags & Node::Flags::camera_anchored) {
				write_line("CameraAnchored,");
			}
			if (node.flags & Node::Flags::unshaded) {
				if (is_particle_emitter_1) {
					// Blizzard reused flag, context-sensitive
					write_line("EmitterUsesMdl,");
				} else {
					write_line("Unshaded,");
				}
			}
			if (node.flags & Node::Flags::sort_primitives_far_z) {
				if (is_particle_emitter_1) {
					// Blizzard reused flag, context-sensitive
					write_line("EmitterUsesTga,");
				} else {
					write_line("SortPrimsFarZ,");
				}
			}
			if (node.flags & Node::Flags::line_emitter) {
				write_line("LineEmitter,");
			}
			if (node.flags & Node::Flags::unfogged) {
				write_line("Unfogged,");
			}
			if (node.flags & Node::Flags::model_space) {
				write_line("ModelSpace,");
			}
			if (node.flags & Node::Flags::xy_quad) {
				write_line("XYQuad,");
			}

			write_node_track(node.KGTR, "Translation");
			write_node_track(node.KGRT, "Rotation");
			write_node_track(node.KGSC, "Scaling");
		}

		template<typename T>
		void start_group(std::string name, T callback) {
			for (size_t i = 0; i < current_indentation; i++) {
				mdl += '\t';
			}
			mdl += name + " {\n";
			current_indentation += 1;
			callback();
			current_indentation -= 1;
			for (size_t i = 0; i < current_indentation; i++) {
				mdl += '\t';
			}
			mdl += "}\n";
		}
	};

	static std::string collision_shape_keyword(CollisionShape::Shape t) {
		switch (t) {
			case CollisionShape::Shape::Box:
				return "Box";
			case CollisionShape::Shape::Plane:
				return "Plane";
			case CollisionShape::Shape::Sphere:
				return "Sphere";
			case CollisionShape::Shape::Cylinder:
				return "Cylinder";
		}
		return "Box";
	}

	static const char* particle_emitter2_filter_mode(uint32_t mode) {
		switch (mode) {
			case 0:
				return "Blend";
			case 1:
				return "Additive";
			case 2:
				return "Modulate";
			case 3:
				return "Modulate2x";
			case 4:
				return "AlphaKey";
		}
		return "Blend";
	}

	static const char* material_filter_mode(uint32_t mode) {
		switch (mode) {
			case 0:
				return "None";
			case 1:
				return "Transparent";
			case 2:
				return "Blend";
			case 3:
				return "Additive";
			case 4:
				return "AddAlpha";
			case 5:
				return "Modulate";
			case 6:
				return "Modulate2x";
		}
		return "None";
	}

	static void write_layer_shading_flags(MDLWriter& mdl, const Layer& layer) {
		if (layer.shading_flags & Layer::ShadingFlags::unshaded) {
			mdl.write_line("Unshaded,");
		}
		if (layer.shading_flags & Layer::ShadingFlags::sphere_environment_map) {
			mdl.write_line("SphereEnvMap,");
		}
		if (layer.shading_flags & Layer::ShadingFlags::two_sided) {
			mdl.write_line("TwoSided,");
		}
		if (layer.shading_flags & Layer::ShadingFlags::unfogged) {
			mdl.write_line("Unfogged,");
		}
		if (layer.shading_flags & Layer::ShadingFlags::no_depth_test) {
			mdl.write_line("NoDepthTest,");
		}
		if (layer.shading_flags & Layer::ShadingFlags::no_depth_set) {
			mdl.write_line("NoDepthSet,");
		}
	}

	// static TextureID id <= slot, when fixed; track form (no slot designator) when animated.
	// The `<= slot` designator only applies to HD layers that bind a texture per PBR slot; SD
	// layers bind a single texture and write the plain `static TextureID id,` form.
	static void write_layer_texture(MDLWriter& mdl, const LayerTexture& texture, const bool with_slot) {
		if (texture.KMTF.tracks.empty()) {
			if (with_slot) {
				mdl.write_line("static TextureID {} <= {},", texture.id, texture.slot);
			} else {
				mdl.write_line("static TextureID {},", texture.id);
			}
		} else {
			mdl.write_track_body(texture.KMTF, "TextureID");
		}
	}

	static void write_layer_extra_tracks(MDLWriter& mdl, const Layer& layer, const uint32_t version) {
		if (!layer.KMTA.tracks.empty() || layer.alpha != 1.f) {
			mdl.write_track(layer.KMTA, "Alpha", layer.alpha);
		}
		if (version < 1000) {
			return;
		}
		if (!layer.KMTE.tracks.empty() || layer.emissive_gain != 1.f) {
			mdl.write_track(layer.KMTE, "EmissiveGain", layer.emissive_gain);
		}
		if (!layer.KFC3.tracks.empty() || layer.fresnel_color != glm::vec3(1.f)) {
			mdl.write_track(layer.KFC3, "FresnelColor", layer.fresnel_color);
		}
		if (!layer.KFCA.tracks.empty() || layer.fresnel_opacity != 0.f) {
			mdl.write_track(layer.KFCA, "FresnelOpacity", layer.fresnel_opacity);
		}
		if (!layer.KFTC.tracks.empty() || layer.fresnel_team_color != 0.f) {
			mdl.write_track(layer.KFTC, "FresnelTeamColor", layer.fresnel_team_color);
		}
	}

	std::string MDX::to_mdl(const uint32_t version) const {
		MDLWriter mdl;

		mdl.start_group("Version", [&]() {
			mdl.write_line("FormatVersion {},", version);
		});

		mdl.start_group(std::format("Model \"{}\"", name), [&]() {
			if (blend_time != 0) {
				mdl.write_line("BlendTime {},", blend_time);
			}
			mdl.write_line("MinimumExtent {{ {}, {}, {} }},", extent.minimum.x, extent.minimum.y, extent.minimum.z);
			mdl.write_line("MaximumExtent {{ {}, {}, {} }},", extent.maximum.x, extent.maximum.y, extent.maximum.z);
			mdl.write_line("BoundsRadius {},", extent.bounds_radius);
		});

		if (!sequences.empty()) {
			mdl.start_group(std::format("Sequences {}", sequences.size()), [&]() {
				for (const auto& i : sequences) {
					mdl.start_group(std::format("Anim \"{}\"", i.name), [&]() {
						mdl.write_line("Interval {{ {}, {} }},", i.start_frame, i.end_frame);
						if (i.movespeed != 0.f) {
							mdl.write_line("MoveSpeed {},", i.movespeed);
						}
						if (i.flags & Sequence::Flags::non_looping) {
							mdl.write_line("NonLooping,");
						}
						if (i.rarity != 0.f) {
							mdl.write_line("Rarity {},", i.rarity);
						}
						if (i.sync_point != 0) {
							mdl.write_line("SyncPoint {},", i.sync_point);
						}
						mdl.write_line("MinimumExtent {{ {}, {}, {} }},", i.extent.minimum.x, i.extent.minimum.y, i.extent.minimum.z);
						mdl.write_line("MaximumExtent {{ {}, {}, {} }},", i.extent.maximum.x, i.extent.maximum.y, i.extent.maximum.z);
						mdl.write_line("BoundsRadius {},", i.extent.bounds_radius);
					});
				}
			});
		}

		if (!global_sequences.empty()) {
			mdl.start_group(std::format("GlobalSequences {}", global_sequences.size()), [&]() {
				for (const auto& i : global_sequences) {
					mdl.write_line("Duration {},", i);
				}
			});
		}

		if (!textures.empty()) {
			mdl.start_group(std::format("Textures {}", textures.size()), [&]() {
				for (const auto& i : textures) {
					mdl.start_group("Bitmap", [&]() {
						mdl.write_line("Image \"{}\",", i.file_name.string());
						if (i.replaceable_id != 0) {
							mdl.write_line("ReplaceableId {},", i.replaceable_id);
						}
						if (i.flags & Texture::Flags::wrap_width) {
							mdl.write_line("WrapWidth,");
						}
						if (i.flags & Texture::Flags::wrap_height) {
							mdl.write_line("WrapHeight,");
						}
					});
				}
			});
		}

		if (!materials.empty()) {
			mdl.start_group(std::format("Materials {}", materials.size()), [&]() {
				for (const auto& material : materials) {
					mdl.start_group("Material", [&]() {
						if (material.priority_plane != 0) {
							mdl.write_line("PriorityPlane {},", material.priority_plane);
						}
						if (material.flags & Material::Flags::constant_color) {
							mdl.write_line("ConstantColor,");
						}
						if (material.flags & Material::Flags::sort_primitives_near_z) {
							mdl.write_line("SortPrimsNearZ,");
						}
						if (material.flags & Material::Flags::sort_primitives_far_z) {
							mdl.write_line("SortPrimsFarZ,");
						}
						if (material.flags & Material::Flags::full_resolution) {
							mdl.write_line("FullResolution,");
						}

						const bool is_hd = !material.layers.empty() && material.layers[0].shader == ShaderType::HD;

						// v900/v1000 record the HD shader name once at the material level.
						if (is_hd && (version == 900 || version == 1000)) {
							mdl.write_line("Shader \"Shader_HD_DefaultUnit\",");
						}

						for (const auto& layer : material.layers) {
							// HD texture slots are only meaningful from v900 on; older targets bind slot 0 only.
							const bool hd_slots = layer.shader == ShaderType::HD && version >= 900;

							mdl.start_group("Layer", [&]() {
								mdl.write_line("FilterMode {},", material_filter_mode(layer.blend_mode));
								write_layer_shading_flags(mdl, layer);

								// v1100+ records the HD shader name per layer instead of per material.
								if (layer.shader == ShaderType::HD && version >= 1100) {
									mdl.write_line("Shader \"Shader_HD_DefaultUnit\",");
								}

								if (layer.texture_animation_id != 0xFFFFFFFF) {
									mdl.write_line("TVertexAnimId {},", layer.texture_animation_id);
								}
								if (layer.coord_id != 0) {
									mdl.write_line("CoordId {},", layer.coord_id);
								}

								if (hd_slots) {
									for (const auto& texture : layer.textures) {
										write_layer_texture(mdl, texture, true);
									}
								} else if (!layer.textures.empty()) {
									write_layer_texture(mdl, layer.textures[0], false);
								}

								write_layer_extra_tracks(mdl, layer, version);
							});
						}
					});
				}
			});
		}

		if (!texture_animations.empty()) {
			mdl.start_group(std::format("TextureAnims {}", texture_animations.size()), [&]() {
				for (const auto& ta : texture_animations) {
					mdl.start_group("TVertexAnim", [&]() {
						if (!ta.KTAT.tracks.empty()) {
							mdl.write_track_body(ta.KTAT, "Translation");
						}
						if (!ta.KTAR.tracks.empty()) {
							mdl.write_track_body(ta.KTAR, "Rotation");
						}
						if (!ta.KTAS.tracks.empty()) {
							mdl.write_track_body(ta.KTAS, "Scaling");
						}
					});
				}
			});
		}

		for (const auto& geoset : geosets) {
			mdl.start_group("Geoset", [&]() {
				mdl.start_group(std::format("Vertices {}", geoset.vertices.size()), [&]() {
					for (const auto& vertex : geoset.vertices) {
						mdl.write_line("{{ {}, {}, {} }},", vertex.x, vertex.y, vertex.z);
					}
				});

				mdl.start_group(std::format("Normals {}", geoset.normals.size()), [&]() {
					for (const auto& normal : geoset.normals) {
						mdl.write_line("{{ {}, {}, {} }},", normal.x, normal.y, normal.z);
					}
				});

				for (const auto& i : geoset.uv_sets) {
					mdl.start_group(std::format("TVertices {}", i.size()), [&]() {
						for (const auto& uv : i) {
							mdl.write_line("{{ {}, {} }},", uv.x, uv.y);
						}
					});
				}

				if (!geoset.tangents.empty() && version >= 900) {
					mdl.start_group(std::format("Tangents {}", geoset.tangents.size()), [&]() {
						for (const auto& tangent : geoset.tangents) {
							mdl.write_line("{{ {}, {}, {}, {} }},", tangent.x, tangent.y, tangent.z, tangent.w);
						}
					});
				}

				if (!geoset.skin.empty() && version >= 900) {
					mdl.start_group(std::format("SkinWeights {}", geoset.skin.size() / 8), [&]() {
						for (size_t i = 0; i < geoset.skin.size() / 8; i++) {
							mdl.write_line(
								"{}, {}, {}, {}, {}, {}, {}, {},",
								geoset.skin[i * 8 + 0],
								geoset.skin[i * 8 + 1],
								geoset.skin[i * 8 + 2],
								geoset.skin[i * 8 + 3],
								geoset.skin[i * 8 + 4],
								geoset.skin[i * 8 + 5],
								geoset.skin[i * 8 + 6],
								geoset.skin[i * 8 + 7]
							);
						}
					});
				}

				if (!geoset.vertex_groups.empty()) {
					mdl.start_group(std::format("VertexGroup"), [&]() {
						for (const auto& vg : geoset.vertex_groups) {
							mdl.write_line("{},", static_cast<uint32_t>(vg));
						}
					});
				}

				mdl.start_group(std::format("Faces 1 {}", geoset.faces.size()), [&]() {
					mdl.start_group("Triangles", [&]() {
						std::string triangles;
						for (size_t i = 0; i < geoset.faces.size(); i++) {
							triangles += std::format("{}", geoset.faces[i]);
							if (i + 1 < geoset.faces.size()) {
								triangles += ", ";
							}
						}
						mdl.write_line("{{ {} }},", triangles);
					});
				});

				if (!geoset.matrix_groups.empty()) {
					uint32_t matrix_cursor = 0;
					mdl.start_group(std::format("Groups {} {}", geoset.matrix_groups.size(), geoset.matrix_indices.size()), [&]() {
						for (uint32_t group_size : geoset.matrix_groups) {
							std::string matrices;
							for (uint32_t i = 0; i < group_size; i++) {
								matrices += std::format("{}", geoset.matrix_indices[matrix_cursor + i]);
								if (i + 1 < group_size) {
									matrices += ", ";
								}
							}
							matrix_cursor += group_size;
							mdl.write_line("Matrices {{ {} }},", matrices);
						}
					});
				}

				mdl.write_line(
					"MinimumExtent {{ {}, {}, {} }},",
					geoset.extent.minimum.x,
					geoset.extent.minimum.y,
					geoset.extent.minimum.z
				);
				mdl.write_line(
					"MaximumExtent {{ {}, {}, {} }},",
					geoset.extent.maximum.x,
					geoset.extent.maximum.y,
					geoset.extent.maximum.z
				);
				mdl.write_line("BoundsRadius {},", geoset.extent.bounds_radius);

				for (const auto& i : geoset.sequence_extents) {
					mdl.start_group("Anim", [&]() {
						mdl.write_line("MinimumExtent {{ {}, {}, {} }},", i.minimum.x, i.minimum.y, i.minimum.z);
						mdl.write_line("MaximumExtent {{ {}, {}, {} }},", i.maximum.x, i.maximum.y, i.maximum.z);
						mdl.write_line("BoundsRadius {},", i.bounds_radius);
					});
				}

				mdl.write_line("MaterialID {},", geoset.material_id);
				mdl.write_line("SelectionGroup {},", geoset.selection_group);
				if (geoset.selection_flags & 4) {
					mdl.write_line("Unselectable,");
				}
				mdl.write_line("LevelOfDetail {},", geoset.lod);
				if (!geoset.lod_name.empty()) {
					mdl.write_line("Name \"{}\",", geoset.lod_name);
				}
			});
		}

		for (const auto& geoset_anim : animations) {
			mdl.start_group("GeosetAnim", [&]() {
				if (geoset_anim.geoset_id == 0xFFFFFFFF) {
					mdl.write_line("GeosetId None,");
				} else {
					mdl.write_line("GeosetId {},", geoset_anim.geoset_id);
				}
				if (geoset_anim.flags & 0x1) {
					mdl.write_line("DropShadow,");
				}
				if (!geoset_anim.KGAO.tracks.empty() || geoset_anim.alpha != 1.f) {
					mdl.write_track(geoset_anim.KGAO, "Alpha", geoset_anim.alpha);
				}
				if (!geoset_anim.KGAC.tracks.empty() || geoset_anim.color != glm::vec3(1.f)) {
					mdl.write_track(geoset_anim.KGAC, "Color", geoset_anim.color);
				}
			});
		}

		for (const auto& bone : bones) {
			mdl.start_group(std::format("Bone \"{}\"", bone.node.name), [&]() {
				mdl.write_node(bone.node);
				if (bone.geoset_id == -1) {
					mdl.write_line("GeosetId Multiple,");
				} else {
					mdl.write_line("GeosetId {},", bone.geoset_id);
				}
				if (bone.geoset_animation_id == -1) {
					mdl.write_line("GeosetAnimId None,");
				} else {
					mdl.write_line("GeosetAnimId {},", bone.geoset_animation_id);
				}
			});
		}

		for (const auto& help_bone : help_bones) {
			mdl.start_group(std::format("Helper \"{}\"", help_bone.name), [&]() {
				mdl.write_node(help_bone);
			});
		}

		for (const auto& light : lights) {
			mdl.start_group(std::format("Light \"{}\"", light.node.name), [&]() {
				mdl.write_node(light.node);
				switch (light.type) {
					case 0:
						mdl.write_line("Omnidirectional,");
						break;
					case 1:
						mdl.write_line("Directional,");
						break;
					case 2:
						mdl.write_line("Ambient,");
						break;
				}
				mdl.write_track(light.KLAS, "AttenuationStart", light.attenuation_start);
				mdl.write_track(light.KLAE, "AttenuationEnd", light.attenuation_end);
				mdl.write_track(light.KLAI, "Intensity", light.intensity);
				mdl.write_track(light.KLAC, "Color", light.color);
				mdl.write_track(light.KLBI, "AmbIntensity", light.ambient_intensity);
				mdl.write_track(light.KLBC, "AmbColor", light.ambient_color);
				if (!light.KLAV.tracks.empty()) {
					mdl.write_track_body(light.KLAV, "Visibility");
				}
			});
		}

		for (const auto& attachment : attachments) {
			mdl.start_group(std::format("Attachment \"{}\"", attachment.node.name), [&]() {
				mdl.write_node(attachment.node);
				mdl.write_line("AttachmentID {},", attachment.attachment_id);
				if (!attachment.path.empty()) {
					mdl.write_line("Path \"{}\",", attachment.path);
				}
				if (!attachment.KATV.tracks.empty()) {
					mdl.write_track_body(attachment.KATV, "Visibility");
				}
			});
		}

		if (!pivots.empty()) {
			mdl.start_group(std::format("PivotPoints {}", pivots.size()), [&]() {
				for (const auto& pivot : pivots) {
					mdl.write_line("{{ {}, {}, {} }},", pivot.x, pivot.y, pivot.z);
				}
			});
		}

		for (const auto& emitter : emitters1) {
			mdl.start_group(std::format("ParticleEmitter \"{}\"", emitter.node.name), [&]() {
				mdl.write_node(emitter.node, true);
				mdl.write_track(emitter.KPEE, "EmissionRate", emitter.emission_rate);
				mdl.write_track(emitter.KPEG, "Gravity", emitter.gravity);
				mdl.write_track(emitter.KPLN, "Longitude", emitter.longitude);
				mdl.write_track(emitter.KPLT, "Latitude", emitter.latitude);
				if (!emitter.KPEV.tracks.empty()) {
					mdl.write_track_body(emitter.KPEV, "Visibility");
				}
				if (!emitter.path.empty()) {
					mdl.write_line("Path \"{}\",", emitter.path);
				}
				mdl.write_track(emitter.KPEL, "LifeSpan", emitter.life_span);
				mdl.write_track(emitter.KPES, "InitVelocity", emitter.speed);
			});
		}

		for (const auto& emitter : emitters2) {
			mdl.start_group(std::format("ParticleEmitter2 \"{}\"", emitter.node.name), [&]() {
				mdl.write_node(emitter.node);

				if (emitter.squirt) {
					mdl.write_line("Squirt,");
				}

				mdl.write_track(emitter.KP2S, "Speed", emitter.speed);
				mdl.write_track(emitter.KP2R, "Variation", emitter.speed_variation);
				mdl.write_track(emitter.KP2L, "Latitude", emitter.latitude);
				mdl.write_track(emitter.KP2G, "Gravity", emitter.gravity);
				if (!emitter.KP2V.tracks.empty()) {
					mdl.write_track_body(emitter.KP2V, "Visibility");
				}

				mdl.write_line("LifeSpan {},", emitter.life_span);
				mdl.write_track(emitter.KP2E, "EmissionRate", emitter.emission_rate);
				mdl.write_track(emitter.KP2W, "Width", emitter.width);
				mdl.write_track(emitter.KP2N, "Length", emitter.length);

				mdl.write_line("{},", particle_emitter2_filter_mode(emitter.filter_mode));

				mdl.write_line("Rows {},", emitter.rows);
				mdl.write_line("Columns {},", emitter.columns);

				switch (emitter.head_or_tail) {
					case 0:
						mdl.write_line("Head,");
						break;
					case 1:
						mdl.write_line("Tail,");
						break;
					case 2:
						mdl.write_line("Both,");
						break;
				}

				mdl.write_line("TailLength {},", emitter.tail_length);
				mdl.write_line("Time {},", emitter.time_middle);

				mdl.start_group("SegmentColor", [&]() {
					mdl.write_line(
						"Color {{ {}, {}, {} }},",
						emitter.start_segment_color.x,
						emitter.start_segment_color.y,
						emitter.start_segment_color.z
					);
					mdl.write_line(
						"Color {{ {}, {}, {} }},",
						emitter.middle_segment_color.x,
						emitter.middle_segment_color.y,
						emitter.middle_segment_color.z
					);
					mdl.write_line(
						"Color {{ {}, {}, {} }},",
						emitter.end_segment_color.x,
						emitter.end_segment_color.y,
						emitter.end_segment_color.z
					);
				});

				mdl.write_line("Alpha {{ {}, {}, {} }},", emitter.segment_alphas.x, emitter.segment_alphas.y, emitter.segment_alphas.z);
				mdl.write_line(
					"ParticleScaling {{ {}, {}, {} }},",
					emitter.segment_scaling.x,
					emitter.segment_scaling.y,
					emitter.segment_scaling.z
				);
				mdl.write_line(
					"LifeSpanUVAnim {{ {}, {}, {} }},",
					emitter.head_intervals.x,
					emitter.head_intervals.y,
					emitter.head_intervals.z
				);
				mdl.write_line(
					"DecayUVAnim {{ {}, {}, {} }},",
					emitter.head_decay_intervals.x,
					emitter.head_decay_intervals.y,
					emitter.head_decay_intervals.z
				);
				mdl.write_line(
					"TailUVAnim {{ {}, {}, {} }},",
					emitter.tail_intervals.x,
					emitter.tail_intervals.y,
					emitter.tail_intervals.z
				);
				mdl.write_line(
					"TailDecayUVAnim {{ {}, {}, {} }},",
					emitter.tail_decay_intervals.x,
					emitter.tail_decay_intervals.y,
					emitter.tail_decay_intervals.z
				);

				mdl.write_line("TextureID {},", emitter.texture_id);
				if (emitter.replaceable_id != 0) {
					mdl.write_line("ReplaceableId {},", emitter.replaceable_id);
				}
				if (emitter.priority_plane != 0) {
					mdl.write_line("PriorityPlane {},", emitter.priority_plane);
				}
			});
		}

		for (const auto& ribbon : ribbons) {
			mdl.start_group(std::format("RibbonEmitter \"{}\"", ribbon.node.name), [&]() {
				mdl.write_node(ribbon.node);
				mdl.write_track(ribbon.KRHA, "HeightAbove", ribbon.height_above);
				mdl.write_track(ribbon.KRHB, "HeightBelow", ribbon.height_below);
				mdl.write_track(ribbon.KRAL, "Alpha", ribbon.alpha);
				mdl.write_track(ribbon.KRCO, "Color", ribbon.color);
				mdl.write_line("LifeSpan {},", ribbon.life_span);
				if (!ribbon.KRTX.tracks.empty()) {
					mdl.write_track_body(ribbon.KRTX, "TextureSlot");
				} else if (ribbon.texture_slot != 0) {
					mdl.write_line("TextureSlot {},", ribbon.texture_slot);
				}
				mdl.write_track(ribbon.KRVS, "Visibility", 1.f);
				mdl.write_line("EmissionRate {},", ribbon.emission_rate);
				mdl.write_line("Rows {},", ribbon.rows);
				mdl.write_line("Columns {},", ribbon.columns);
				mdl.write_line("MaterialID {},", ribbon.material_id);
				if (ribbon.gravity != 0.f) {
					mdl.write_line("Gravity {},", ribbon.gravity);
				}
			});
		}

		for (const auto& event_object : event_objects) {
			mdl.start_group(std::format("EventObject \"{}\"", event_object.node.name), [&]() {
				mdl.write_node(event_object.node);

				mdl.start_group(std::format("EventTrack {}", event_object.times.size()), [&]() {
					if (event_object.global_sequence_id != -1) {
						mdl.write_line("GlobalSeqId {},", event_object.global_sequence_id);
					}
					for (const auto& track : event_object.times) {
						mdl.write_line("{},", track);
					}
				});
			});
		}

		for (const auto& collision_shape : collision_shapes) {
			mdl.start_group(std::format("CollisionShape \"{}\"", collision_shape.node.name), [&]() {
				mdl.write_node(collision_shape.node);

				mdl.write_line("{},", collision_shape_keyword(collision_shape.type));

				const bool single_vertex = collision_shape.type == CollisionShape::Shape::Sphere;
				const size_t vertex_count = single_vertex ? 1 : 2;
				mdl.start_group(std::format("Vertices {}", vertex_count), [&]() {
					for (size_t i = 0; i < vertex_count; i++) {
						mdl.write_line(
							"{{ {}, {}, {} }},",
							collision_shape.vertices[i].x,
							collision_shape.vertices[i].y,
							collision_shape.vertices[i].z
						);
					}
				});
				if (collision_shape.type == CollisionShape::Shape::Sphere || collision_shape.type == CollisionShape::Shape::Cylinder) {
					mdl.write_line("BoundsRadius {},", collision_shape.radius);
				}
			});
		}

		for (const auto& camera : cameras) {
			mdl.start_group(std::format("Camera \"{}\"", camera.name), [&]() {
				mdl.write_line("Position {{ {}, {}, {} }},", camera.position.x, camera.position.y, camera.position.z);
				if (!camera.KCTR.tracks.empty()) {
					mdl.write_track_body(camera.KCTR, "Translation");
				}
				if (!camera.KCRL.tracks.empty()) {
					mdl.write_track_body(camera.KCRL, "Rotation");
				}
				mdl.write_line("FieldOfView {},", camera.field_of_view);
				mdl.write_line("FarClip {},", camera.far_clip);
				mdl.write_line("NearClip {},", camera.near_clip);
				mdl.start_group("Target", [&]() {
					mdl.write_line(
						"Position {{ {}, {}, {} }},",
						camera.target_position.x,
						camera.target_position.y,
						camera.target_position.z
					);
					if (!camera.KTTR.tracks.empty()) {
						mdl.write_track_body(camera.KTTR, "Translation");
					}
				});
			});
		}

		if (!bind_poses.empty() && version >= 900) {
			mdl.start_group("BindPose", [&]() {
				const size_t matrix_count = bind_poses.size() / 12;
				mdl.start_group(std::format("Matrices {}", matrix_count), [&]() {
					for (size_t i = 0; i < matrix_count; i++) {
						const float* m = &bind_poses[i * 12];
						mdl.write_line(
							"{{ {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {} }},",
							m[0],
							m[1],
							m[2],
							m[3],
							m[4],
							m[5],
							m[6],
							m[7],
							m[8],
							m[9],
							m[10],
							m[11]
						);
					}
				});
			});
		}

		for (const auto& facefx : facefxes) {
			mdl.start_group(std::format("FaceFX \"{}\"", facefx.name), [&]() {
				mdl.write_line("Path \"{}\",", facefx.path.string());
			});
		}

		for (const auto& corn : corn_emitters) {
			mdl.start_group(std::format("ParticleEmitterPopcorn \"{}\"", corn.node.name), [&]() {
				mdl.write_node(corn.node);
				// TODO: decode PopcornFX data fields (payload kept opaque for now)
			});
		}

		return mdl.mdl;
	}
} // namespace mdx
