module MDX;

import std;
import Timer;
import <nlohmann/json.hpp>;
using json = nlohmann::json;
import <glm/glm.hpp>;

namespace mdx {
	/// A minimal utility wrapper around an std::string that manages newlines, indentation and closing braces
	struct MDLWriter {
		std::string mdl;
		size_t current_indentation = 0;

		template <typename... Args>
		void write_line(std::format_string<Args...> fmt, Args&&... args) {
			for (size_t i = 0; i < current_indentation; i++) {
				mdl += '\t';
			}
			mdl += std::vformat(fmt.get(), std::make_format_args(args...));
			mdl += '\n';
		}

		template <typename T>
		void write_track(const TrackHeader<T>& track_header, std::string name, T static_value) {
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
				start_group(name, [&]() {
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

					write_line("GlobalSeqId {},", track_header.global_sequence_ID);

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

						if (track_header.interpolation_type == InterpolationType::hermite || track_header.interpolation_type == InterpolationType::bezier) {
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
		}

		void write_node(const Node& node) {
			write_line("ObjectId {},", node.id);
			write_line("Parent {},", node.parent_id);

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
			for (size_t i = 0; i < current_indentation; i++) {
				mdl += '\t';
			}
			mdl += "}\n";
		}
	};

	std::string MDX::to_mdl() {
		json j;
		j["Version"]["FormatVersion"] = 1000;
		
		j["Model"]["BlendTime"] = blend_time;
		j["Model"]["MinimumExtent"] = {
			extent.minimum.x,
			extent.minimum.y,
			extent.minimum.z,
		};
		j["Model"]["MaximumExtent"] = {
			extent.maximum.x,
			extent.maximum.y,
			extent.maximum.z,
		};

		j["Sequences"] = json::array();
		for (const auto& i : sequences) {
			json sequence;
			sequence["Interval"] = { i.start_frame, i.end_frame };
			sequence["Movespeed"] = i.movespeed;
			sequence["SyncPoint"] = i.sync_point;
			sequence["NonLooping"] = i.flags & Sequence::Flags::non_looping;
			sequence["Rarity"] = i.rarity;
			sequence["MinimumExtent"] = {
				i.extent.minimum.x,
				i.extent.minimum.y,
				i.extent.minimum.z,
			};
			sequence["MaximumExtent"] = {
				i.extent.maximum.x,
				i.extent.maximum.y,
				i.extent.maximum.z,
			};
			sequence["BoundRadius"] = i.extent.bounds_radius;
			j["Sequences"].push_back(sequence);
		}

		std::ofstream file("C:/Users/User/Desktop/MDX.json");
		file << j;
		file.close();

		Timer timer;
		MDLWriter mdl;

		mdl.start_group("Version", [&]() {
			mdl.write_line("FormatVersion 1000,");
		});

		mdl.start_group(std::format("Model \"{}\"", name), [&]() {
			mdl.write_line("BlendTime {},", blend_time);
			mdl.write_line("MinimumExtent {{ {}, {}, {} }},", extent.minimum.x, extent.minimum.y, extent.minimum.z);
			mdl.write_line("MaximumExtent {{ {}, {}, {} }},", extent.maximum.x, extent.maximum.y, extent.maximum.z);
		});

		mdl.start_group(std::format("Sequences {}", sequences.size()), [&]() {
			for (const auto& i : sequences) {
				mdl.start_group(std::format("Anim \"{}\"", i.name), [&]() {
					mdl.write_line("Interval {{ {}, {} }},", i.start_frame, i.end_frame);
					mdl.write_line("Movespeed {},", i.movespeed);
					mdl.write_line("SyncPoint {},", i.sync_point);

					if (i.flags & Sequence::Flags::non_looping) {
						mdl.write_line("NonLooping,");
					}

					mdl.write_line("Rarity {},", i.rarity);
					mdl.write_line("MinimumExtent {{ {}, {}, {}, }},", i.extent.minimum.x, i.extent.minimum.y, i.extent.minimum.z);
					mdl.write_line("MinimumExtent {{ {}, {}, {}, }},", i.extent.maximum.x, i.extent.maximum.y, i.extent.maximum.z);
					mdl.write_line("BoundRadius {},", i.extent.bounds_radius);
				});
			}
		});

		mdl.start_group(std::format("GlobalSequences {}", global_sequences.size()), [&]() {
			for (const auto& i : global_sequences) {
				mdl.write_line("Duration {},", i);
			}
		});

		mdl.start_group(std::format("Textures {}", textures.size()), [&]() {
			for (const auto& i : textures) {
				mdl.start_group("Bitmap", [&]() {
					mdl.write_line("Image \"{}\",", i.file_name.string());
					mdl.write_line("ReplaceableId {},", i.replaceable_id);
					if (i.flags & Texture::Flags::wrap_width) {
						mdl.write_line("WrapWidth");
					}
					if (i.flags & Texture::Flags::wrap_height) {
						mdl.write_line("WrapHeight");
					}
				});
			}
		});

		mdl.start_group(std::format("Materials {}", materials.size()), [&]() {
			for (const auto& material : materials) {
				mdl.start_group("Material", [&]() {
					if (material.layers[0].hd) {
						mdl.write_line("Shader \"Shader_HD_DefaultUnit\",");
						const auto& layer = material.layers[0];
						for (int i = 0; i < 6; i++) {
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

							mdl.write_track(layer.textures.at(i).KMTF, "TextureID", layer.textures.at(i).id);
							mdl.write_track(layer.KMTA, "Alpha", layer.alpha);
							mdl.write_track(layer.KMTE, "EmissiveGain", layer.emissive_gain);
							mdl.write_track(layer.KFC3, "FresnelColor", layer.fresnel_color);
							mdl.write_track(layer.KFCA, "FresnelAlpha", layer.fresnel_opacity);
							mdl.write_track(layer.KFTC, "FresnelTeamColor", layer.fresnel_team_color);
						}
					} else {
						mdl.write_line("Shader \"\",");

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

								mdl.write_track(layer.textures.at(0).KMTF, "TextureID", layer.textures.at(0).id);
								mdl.write_track(layer.KMTA, "Alpha", layer.alpha);
								mdl.write_track(layer.KMTE, "EmissiveGain", layer.emissive_gain);
								mdl.write_track(layer.KFC3, "FresnelColor", layer.fresnel_color);
								mdl.write_track(layer.KFCA, "FresnelAlpha", layer.fresnel_opacity);
								mdl.write_track(layer.KFTC, "FresnelTeamColor", layer.fresnel_team_color);
							});
						}
					}
				});
			}
		});

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

				mdl.start_group(std::format("Tangents {}", geoset.tangents.size()), [&]() {
					for (const auto& tangent : geoset.tangents) {
						mdl.write_line("{{ {}, {}, {}, {} }},", tangent.x, tangent.y, tangent.z, tangent.w);
					}
				});

				mdl.start_group(std::format("SkinWeights {}", geoset.skin.size() / 8), [&]() {
					for (size_t i = 0; i < geoset.skin.size() / 8; i++) {
						mdl.write_line("{}, {}, {}, {}, {}, {}, {}, {},",
												   geoset.skin[i * 8],
												   geoset.skin[i * 8 + 1],
												   geoset.skin[i * 8 + 2],
												   geoset.skin[i * 8 + 3],
												   geoset.skin[i * 8 + 4],
												   geoset.skin[i * 8 + 5],
												   geoset.skin[i * 8 + 6],
												   geoset.skin[i * 8 + 7]);
					}
				});

				mdl.start_group(std::format("Faces {}", geoset.faces.size()), [&]() {
					mdl.start_group("Triangles", [&]() { // Y'all mfs gonna be having triangles, I ain't in the quad business
						std::string triangles;
						for (const auto& face : geoset.faces) {
							triangles += std::format("{}, ", face);
						}

						mdl.write_line("{{ {} }}", triangles);
					});
				});

				mdl.write_line("MinimumExtent {{ {}, {}, {} }},", geoset.extent.minimum.x, geoset.extent.minimum.z, geoset.extent.minimum.z);
				mdl.write_line("MaximumExtent {{ {}, {}, {} }},", geoset.extent.maximum.x, geoset.extent.maximum.z, geoset.extent.maximum.z);
				mdl.write_line("BoundsRadius {},", geoset.extent.bounds_radius);

				for (const auto& i : geoset.sequence_extents) {
					mdl.start_group("Anim", [&]() {
						mdl.write_line("MinimumExtent {{ {}, {}, {} }},", i.minimum.x, i.minimum.z, i.minimum.z);
						mdl.write_line("MaximumExtent {{ {}, {}, {} }},", i.maximum.x, i.maximum.z, i.maximum.z);
						mdl.write_line("BoundsRadius {},", i.bounds_radius);
					});
				}

				mdl.write_line("MaterialID {},", geoset.material_id);
				mdl.write_line("SelectionGroup {},", geoset.selection_group);
				mdl.write_line("LevelOfDetail 0,");
				mdl.write_line("Name {},", geoset.lod_name);
			});
		}

		for (const auto& geoset_anim : animations) {
			mdl.start_group("GeosetAnim", [&]() {
				mdl.write_track(geoset_anim.KGAO, "Alpha", geoset_anim.alpha);
				mdl.write_track(geoset_anim.KGAC, "Color", geoset_anim.color);
				mdl.write_line("GeosetId {}", geoset_anim.geoset_id);
			});
		}

		for (const auto& bone : bones) {
			mdl.start_group(std::format("Bone \"{}\"", bone.node.name), [&]() {
				mdl.write_line("GeosetId {},", bone.geoset_id); // The MDL has "Multiple" as value for some reason
				mdl.write_line("GeosetAnimId {},", bone.geoset_animation_id); // And this one has "None"

				mdl.write_node(bone.node);
			});
		}

		for (const auto& help_bone : help_bones) {
			mdl.start_group(std::format("Helper \"{}\"", help_bone.name), [&]() {
				mdl.write_node(help_bone);
			});
		}

		for (const auto& attachment : attachments) {
			mdl.start_group(std::format("Helper \"{}\"", attachment.node.name), [&]() {
				mdl.write_node(attachment.node);
				mdl.write_line("AttachmentID {},", attachment.attachment_id);
				mdl.write_track(attachment.KATV, "Visibility", 0.f); // dunno
			});
		}

		mdl.start_group(std::format("PivotPoints \"{}\"", pivots.size()), [&]() {
			for (const auto& pivot : pivots) {
				mdl.write_line("{{ {}, {}, {} }},", pivot.x, pivot.y, pivot.z);
			}
		});

		for (const auto& emitter : emitters2) {
			mdl.start_group(std::format("ParticleEmitter2 \"{}\"", emitter.node.name), [&]() {
				mdl.write_node(emitter.node);

				mdl.write_track(emitter.KP2S, "Speed", emitter.speed);
				mdl.write_track(emitter.KP2R, "Variation", emitter.variation);
				mdl.write_track(emitter.KP2L, "Latitude", emitter.latitude);
				mdl.write_track(emitter.KP2G, "Gravity", emitter.gravity);
				if (emitter.squirt) {
					mdl.write_line("Squirt,");
				}
				mdl.write_track(emitter.KP2V, "Visibility", 0.f); // ToDo static value
				mdl.write_line("Lifespan {},", emitter.life_span);
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

				mdl.write_line("Rows {},", emitter.rows);
				mdl.write_line("Columns {},", emitter.columns);

				if (emitter.head_or_tail == 0) {
					mdl.write_line("Head,");
				} else if (emitter.head_or_tail == 0) {
					mdl.write_line("Tail,");
				} else {
					mdl.write_line("Both,");
				}

				mdl.write_line("TailLength {},", emitter.tail_length);
				mdl.write_line("Time {},", emitter.time_middle);

				mdl.start_group("SegmentColor", [&]() {
					mdl.write_line("Color {{ {}, {}, {}  }},", emitter.start_segment_color.x, emitter.start_segment_color.y, emitter.start_segment_color.z);
					mdl.write_line("Color {{ {}, {}, {}  }},", emitter.middle_segment_color.x, emitter.middle_segment_color.y, emitter.middle_segment_color.z);
					mdl.write_line("Color {{ {}, {}, {}  }},", emitter.end_segment_color.x, emitter.end_segment_color.y, emitter.end_segment_color.z);
				});

				mdl.write_line("Alpha {{ {}, {}, {}  }},", emitter.segment_alphas.x, emitter.segment_alphas.y, emitter.segment_alphas.z);
				mdl.write_line("ParticleScaling {{ {}, {}, {}  }},", emitter.segment_scaling.x, emitter.segment_scaling.y, emitter.segment_scaling.z);
				mdl.write_line("LifeSpanUVAnim {{ {}, {}, {}  }},", emitter.head_intervals.x, emitter.head_intervals.y, emitter.head_intervals.z);
				mdl.write_line("DecayUVAnim {{ {}, {}, {}  }},", emitter.head_decay_intervals.x, emitter.head_decay_intervals.y, emitter.head_decay_intervals.z);
				mdl.write_line("TailUVAnim {{ {}, {}, {}  }},", emitter.tail_intervals.x, emitter.tail_intervals.y, emitter.tail_intervals.z);
				mdl.write_line("TailDecayUVAnim {{ {}, {}, {}  }},", emitter.tail_decay_intervals.x, emitter.tail_decay_intervals.y, emitter.tail_decay_intervals.z);

				mdl.write_line("TextureID {},", emitter.texture_id);
				mdl.write_line("PriorityPlane {},", emitter.priority_plane);
			});
		}

		for (const auto& event_object : event_objects) {
			mdl.start_group(std::format("EventObject \"{}\"", event_object.node.name), [&]() {
				mdl.write_node(event_object.node);

				mdl.start_group(std::format("EventTrack {}", event_object.times.size()), [&]() {
					for (const auto& track : event_object.times) {
						mdl.write_line("{},", track);
					}
				});
			});
		}

		for (const auto& collision_shape : collision_shapes) {
			mdl.start_group(std::format("CollisionShape \"{}\"", collision_shape.node.name), [&]() {
				mdl.write_node(collision_shape.node);

				switch (collision_shape.type) {
					case CollisionShape::Shape::Box:
						mdl.write_line("Cube,");
						mdl.start_group("Vertices 2", [&]() {
							mdl.write_line("{{ {}, {}, {}  }},", collision_shape.vertices[0].x, collision_shape.vertices[0].y, collision_shape.vertices[0].z);
							mdl.write_line("{{ {}, {}, {}  }},", collision_shape.vertices[1].x, collision_shape.vertices[1].y, collision_shape.vertices[1].z);
						});
						break;
					case CollisionShape::Shape::Plane:
						mdl.write_line("Plane,");
						mdl.start_group("Vertices 2", [&]() {
							mdl.write_line("{{ {}, {}, {}  }},", collision_shape.vertices[0].x, collision_shape.vertices[0].y, collision_shape.vertices[0].z);
							mdl.write_line("{{ {}, {}, {}  }},", collision_shape.vertices[1].x, collision_shape.vertices[1].y, collision_shape.vertices[1].z);
						});
						break;
					case CollisionShape::Shape::Sphere:
						mdl.write_line("Sphere,");
						mdl.start_group("Vertices 1", [&]() {
							mdl.write_line("{{ {}, {}, {}  }},", collision_shape.vertices[0].x, collision_shape.vertices[0].y, collision_shape.vertices[0].z);
						});
						mdl.write_line("BoundsRadius {},", collision_shape.radius);
						break;
					case CollisionShape::Shape::Cylinder:
						mdl.write_line("Cylinder,");
						mdl.start_group("Vertices 2", [&]() {
							mdl.write_line("{{ {}, {}, {}  }},", collision_shape.vertices[0].x, collision_shape.vertices[0].y, collision_shape.vertices[0].z);
							mdl.write_line("{{ {}, {}, {}  }},", collision_shape.vertices[1].x, collision_shape.vertices[1].y, collision_shape.vertices[1].z);
						});
						mdl.write_line("BoundsRadius {},", collision_shape.radius);
						break;
				}
			});
		}
		std::println("Elapsed {}ms", timer.elapsed_ms());
		return mdl.mdl;
	}
} // namespace mdx