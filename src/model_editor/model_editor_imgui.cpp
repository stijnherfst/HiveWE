#include "model_editor_glwidget.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>

#include <qt_imgui/qt_imGui.h>

import std;
import MDX;
import <imgui.h>;
import <imgui_internal.h>;

namespace {
	const char* blend_mode_name(const uint32_t mode) {
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
				return "Add Alpha";
			case 5:
				return "Modulate";
			case 6:
				return "Modulate 2x";
			default:
				return "Unknown";
		}
	}

	const char* shader_type_name(const mdx::ShaderType shader) {
		switch (shader) {
			case mdx::ShaderType::SD:
				return "SD";
			case mdx::ShaderType::HD:
				return "HD";
			case mdx::ShaderType::SDOnHD:
				return "SD on HD";
			default:
				return "Unknown";
		}
	}

	const char* layer_slot_name(const uint32_t slot) {
		switch (slot) {
			case 0:
				return "Diffuse";
			case 1:
				return "Normal";
			case 2:
				return "ORM";
			case 3:
				return "Emissive";
			case 4:
				return "Team Color";
			case 5:
				return "Environment";
			default:
				return "Slot";
		}
	}

	const char* interpolation_name(const mdx::InterpolationType type) {
		switch (type) {
			case mdx::InterpolationType::none:
				return "none";
			case mdx::InterpolationType::linear:
				return "linear";
			case mdx::InterpolationType::hermite:
				return "hermite";
			case mdx::InterpolationType::bezier:
				return "bezier";
			default:
				return "?";
		}
	}

	/// Joins the labels of every set bit into a comma-separated string, or "none".
	std::string join_flags(std::initializer_list<std::pair<uint32_t, const char*>> table, const uint32_t flags) {
		std::string result;
		for (const auto& [bit, label] : table) {
			if (flags & bit) {
				if (!result.empty()) {
					result += ", ";
				}
				result += label;
			}
		}
		return result.empty() ? "none" : result;
	}

	/// The primary kind of a node, derived from its type bits.
	const char* node_type_label(const int flags) {
		const auto f = static_cast<uint32_t>(flags);
		if (f & mdx::Node::bone) {
			return "Bone";
		}
		if (f & mdx::Node::light) {
			return "Light";
		}
		if (f & mdx::Node::attachment) {
			return "Attachment";
		}
		if (f & mdx::Node::collision_shape) {
			return "Collision Shape";
		}
		if (f & mdx::Node::ribbon_emitter) {
			return "Ribbon Emitter";
		}
		if (f & mdx::Node::emitter) {
			return "Particle Emitter";
		}
		if (f & mdx::Node::object) {
			return "Helper";
		}
		return "Node";
	}

	std::string node_behavior_string(const int flags) {
		return join_flags(
			{
				{mdx::Node::dont_inherit_translation, "no inherit translation"},
				{mdx::Node::dont_inherit_rotation, "no inherit rotation"},
				{mdx::Node::dont_inherit_scaling, "no inherit scaling"},
				{mdx::Node::billboarded, "billboarded"},
				{mdx::Node::billboarded_lock_x, "billboard lock x"},
				{mdx::Node::billboarded_lock_y, "billboard lock y"},
				{mdx::Node::billboarded_lock_z, "billboard lock z"},
				{mdx::Node::camera_anchored, "camera anchored"},
				{mdx::Node::unshaded, "unshaded"},
				{mdx::Node::sort_primitives_far_z, "sort far z"},
				{mdx::Node::unfogged, "unfogged"},
				{mdx::Node::model_space, "model space"},
				{mdx::Node::xy_quad, "xy quad"},
			},
			static_cast<uint32_t>(flags)
		);
	}

	std::string material_flags_string(const uint32_t flags) {
		return join_flags(
			{
				{mdx::Material::constant_color, "constant color"},
				{mdx::Material::sort_primitives_near_z, "sort near z"},
				{mdx::Material::sort_primitives_far_z, "sort far z"},
				{mdx::Material::full_resolution, "full resolution"},
			},
			flags
		);
	}

	std::string layer_shading_flags_string(const uint32_t flags) {
		return join_flags(
			{
				{mdx::Layer::unshaded, "unshaded"},
				{mdx::Layer::sphere_environment_map, "sphere env map"},
				{mdx::Layer::two_sided, "two sided"},
				{mdx::Layer::unfogged, "unfogged"},
				{mdx::Layer::no_depth_test, "no depth test"},
				{mdx::Layer::no_depth_set, "no depth set"},
			},
			flags
		);
	}

	std::string texture_display_name(const mdx::Texture& texture) {
		if (texture.replaceable_id != 0) {
			const auto it = mdx::replaceable_id_to_texture.find(texture.replaceable_id);
			if (it != mdx::replaceable_id_to_texture.end()) {
				return it->second;
			}
			return std::format("Replaceable ID {}", texture.replaceable_id);
		}
		if (texture.file_name.empty()) {
			return "(none)";
		}
		return texture.file_name.string();
	}

	// Queries the dimensions of an already-uploaded GL texture's base level.
	std::pair<int, int> texture_size(const GLuint id) {
		int width = 0;
		int height = 0;
		if (id != 0) {
			glGetTextureLevelParameteriv(id, 0, GL_TEXTURE_WIDTH, &width);
			glGetTextureLevelParameteriv(id, 0, GL_TEXTURE_HEIGHT, &height);
		}
		return {width, height};
	}

	template<typename T>
	std::string track_value_string(const T& value) {
		if constexpr (std::is_same_v<T, float>) {
			return std::format("{:.4f}", value);
		} else if constexpr (std::is_same_v<T, uint32_t>) {
			return std::format("{}", value);
		} else if constexpr (std::is_same_v<T, glm::vec3>) {
			return std::format("({:.3f}, {:.3f}, {:.3f})", value.x, value.y, value.z);
		} else if constexpr (std::is_same_v<T, glm::quat>) {
			return std::format("({:.3f}, {:.3f}, {:.3f}, {:.3f})", value.x, value.y, value.z, value.w);
		} else {
			return "?";
		}
	}

	// Renders one animation track as an expandable summary; expanding shows every keyframe.
	template<typename T>
	void render_track(const char* label, const mdx::TrackHeader<T>& track) {
		if (track.tracks.empty()) {
			ImGui::TextDisabled("%s: none", label);
			return;
		}

		const bool tangents =
			track.interpolation_type == mdx::InterpolationType::hermite || track.interpolation_type == mdx::InterpolationType::bezier;

		if (ImGui::TreeNode(label, "%s: %s (%zu keys)", label, interpolation_name(track.interpolation_type), track.tracks.size())) {
			if (track.global_sequence_ID != -1) {
				ImGui::Text("Global sequence: %d", track.global_sequence_ID);
			}

			const int columns = tangents ? 4 : 2;
			if (ImGui::BeginTable("keys", columns, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Frame");
				ImGui::TableSetupColumn("Value");
				if (tangents) {
					ImGui::TableSetupColumn("In tan");
					ImGui::TableSetupColumn("Out tan");
				}
				ImGui::TableHeadersRow();

				for (const auto& key : track.tracks) {
					ImGui::TableNextColumn();
					ImGui::Text("%d", key.frame);
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(track_value_string(key.value).c_str());
					if (tangents) {
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(track_value_string(key.inTan).c_str());
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(track_value_string(key.outTan).c_str());
					}
				}
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}
	}

	void render_node_tracks(const mdx::Node& node) {
		render_track("Translation", node.KGTR);
		render_track("Rotation", node.KGRT);
		render_track("Scaling", node.KGSC);
	}
} // namespace

void ModelEditorGLWidget::render_imgui() {
	QtImGui::newFrame(ref);

	// No dockspace: windows float inside the widget and can be docked together into floating groups,
	// but nothing docks to the widget edges. This keeps the 3D view unobstructed (no full-screen host
	// window) and lets the camera receive mouse input everywhere a panel isn't covering.

	// The default layout groups all the data panels into one floating tab group, with Controls and
	// Animation as separate floating windows. Built once when there is no remembered layout.
	static const char* const grouped_windows[] =
		{"Overview", "Geosets", "Materials", "Textures", "Sequences", "Nodes", "Emitters", "Other"};
	if (build_default_layout) {
		build_default_layout = false;

		const ImGuiID group = ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_None);
		ImGui::DockBuilderSetNodePos(group, ImVec2(360.0f, 60.0f));
		ImGui::DockBuilderSetNodeSize(group, ImVec2(540.0f, 620.0f));
		for (const char* name : grouped_windows) {
			ImGui::DockBuilderDockWindow(name, group);
		}
		ImGui::DockBuilderFinish(group);
	}

	// Hide the drop-down tab-list ("window menu") button on every dock node's tab bar.
	{
		ImGuiContext& g = *ImGui::GetCurrentContext();
		for (const ImGuiStoragePair& pair : g.DockContext.Nodes.Data) {
			if (auto* node = static_cast<ImGuiDockNode*>(pair.val_p)) {
				node->LocalFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;
				node->UpdateMergedFlags();
			}
		}
	}

	constexpr auto table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;

	mdx::MDX& model = *mesh->mdx;

	// Cascade first-run positions for the free-floating (non-grouped) windows so they don't stack at
	// the top-left. Ignored once a window has a remembered position or is docked into a group.
	int window_cascade = 0;
	const auto place_next_window = [&window_cascade]() {
		ImGui::SetNextWindowPos(ImVec2(20.0f + window_cascade * 28.0f, 20.0f + window_cascade * 28.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(330.0f, 260.0f), ImGuiCond_FirstUseEver);
		window_cascade++;
	};

	place_next_window();
	if (ImGui::Begin("Controls")) {
		ImGui::Text("FPS: %.1f", 1.0 / delta);

		ImGui::SeparatorText("File");
		// Edit MDL starts a hot-reload session: write the model to a temp .mdl, open it in the default
		// editor, and watch it so external edits reload the model live (see reload_from_mdl()).
		if (ImGui::Button("Edit MDL")) {
			const QString path =
				QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + QString::fromStdString(model.name) + ".mdl";

			const auto mdl = model.to_mdl();
			std::ofstream file(path.toStdString(), std::ios::binary);
			file.write(mdl.data(), mdl.size());
			file.close();

			if (!hot_reload_path.empty()) {
				mdl_watcher->removePath(QString::fromStdString(hot_reload_path));
			}
			hot_reload_path = path.toStdString();
			mdl_watcher->addPath(path);

			QDesktopServices::openUrl(QUrl(path, QUrl::TolerantMode));
		}
		ImGui::SameLine();
		if (ImGui::Button("Save to MDX")) {
			const QString file_name = QFileDialog::getSaveFileName(
				this,
				"Save MDX",
				QStandardPaths::writableLocation(QStandardPaths::TempLocation),
				"MDX (*.mdx *.MDX)"
			);

			if (!file_name.isEmpty()) {
				const auto mdx_data = model.to_mdx();
				std::ofstream file(file_name.toStdString(), std::ios::binary);
				if (file) {
					file.write(reinterpret_cast<const char*>(mdx_data.buffer.data()), mdx_data.buffer.size());
				}
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Save to MDL")) {
			const QString file_name = QFileDialog::getSaveFileName(
				this,
				"Save MDL",
				QStandardPaths::writableLocation(QStandardPaths::TempLocation),
				"MDL (*.mdl *.MDL)"
			);

			if (!file_name.isEmpty()) {
				const auto mdl = model.to_mdl();
				std::ofstream file(file_name.toStdString(), std::ios::binary);
				if (file) {
					file.write(mdl.data(), mdl.size());
				}
			}
		}

		ImGui::SeparatorText("View");
		if (ImGui::Button("Recalculate Extents")) {
			calculate_animated_extents(mdx);
			recenter_camera();
		}
		ImGui::Text("Draw:");
		ImGui::SameLine();
		ImGui::Checkbox("Box", &draw_extents_box);
		ImGui::SameLine();
		ImGui::Checkbox("Sphere", &draw_extents_sphere);
		ImGui::SameLine();
		ImGui::Checkbox("Grid", &draw_grid);

		ImGui::SeparatorText("Animation");
		if (ImGui::Button(animation_paused ? "Play" : "Pause", ImVec2(60, 0))) {
			animation_paused = !animation_paused;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::BeginCombo("##sequence", model.sequences[skeleton.sequence_index].name.c_str())) {
			for (size_t i = 0; i < model.sequences.size(); i++) {
				if (ImGui::Selectable(model.sequences[i].name.c_str(), i == skeleton.sequence_index)) {
					skeleton.set_sequence(static_cast<int>(i));
					recenter_camera();
				}
				if (i == skeleton.sequence_index) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		const auto& sequence = model.sequences[skeleton.sequence_index];
		int frame = skeleton.current_frame;
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::SliderInt("##frame", &frame, static_cast<int>(sequence.start_frame), static_cast<int>(sequence.end_frame), "Frame %d")) {
			skeleton.current_frame = frame;
		}
		ImGui::Text("Range: %u - %u", sequence.start_frame, sequence.end_frame);
		ImGui::Text("Looping: %s", sequence.flags == mdx::Sequence::looping ? "yes" : "no");
	}
	ImGui::End();

	place_next_window();
	if (ImGui::Begin("Overview")) {
		ImGui::SeparatorText("Model");
		ImGui::Text("Name: %s", model.name.c_str());
		ImGui::Text("Version: %u", model.version);
		ImGui::Text("Blend time: %u ms", model.blend_time);
		if (!model.animation_filename.empty()) {
			ImGui::Text("Animation file: %s", model.animation_filename.c_str());
		}

		size_t vertices = 0;
		size_t triangles = 0;
		for (const auto& geoset : model.geosets) {
			vertices += geoset.vertices.size();
			triangles += geoset.faces.size() / 3;
		}
		ImGui::Text("Vertices: %zu", vertices);
		ImGui::Text("Triangles: %zu", triangles);

		ImGui::SeparatorText("Contents");
		if (ImGui::BeginTable("counts", 2, table_flags)) {
			const auto row = [](const char* label, size_t count) {
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(label);
				ImGui::TableNextColumn();
				ImGui::Text("%zu", count);
			};
			row("Geosets", model.geosets.size());
			row("Materials", model.materials.size());
			row("Textures", model.textures.size());
			row("Texture animations", model.texture_animations.size());
			row("Sequences", model.sequences.size());
			row("Global sequences", model.global_sequences.size());
			row("Geoset animations", model.animations.size());
			row("Bones", model.bones.size());
			row("Helpers", model.help_bones.size());
			row("Lights", model.lights.size());
			row("Attachments", model.attachments.size());
			row("Particle emitters", model.emitters1.size());
			row("Particle emitters 2", model.emitters2.size());
			row("Ribbon emitters", model.ribbons.size());
			row("Cameras", model.cameras.size());
			row("Collision shapes", model.collision_shapes.size());
			row("Event objects", model.event_objects.size());
			row("Pivots", model.pivots.size());
			ImGui::EndTable();
		}

		ImGui::SeparatorText("Extent");
		const auto& e = model.extent;
		ImGui::Text("Min: %.1f, %.1f, %.1f", e.minimum.x, e.minimum.y, e.minimum.z);
		ImGui::Text("Max: %.1f, %.1f, %.1f", e.maximum.x, e.maximum.y, e.maximum.z);
		ImGui::Text("Bounds radius: %.1f", e.bounds_radius);
	}
	ImGui::End();

	place_next_window();
	if (ImGui::Begin("Geosets")) {
		if (ImGui::BeginTable("geosets", 8, table_flags)) {
			ImGui::TableSetupColumn("#");
			ImGui::TableSetupColumn("Vertices");
			ImGui::TableSetupColumn("Triangles");
			ImGui::TableSetupColumn("Material");
			ImGui::TableSetupColumn("LOD");
			ImGui::TableSetupColumn("UV sets");
			ImGui::TableSetupColumn("HD");
			ImGui::TableSetupColumn("Radius");
			ImGui::TableHeadersRow();

			for (size_t i = 0; i < model.geosets.size(); i++) {
				const auto& geoset = model.geosets[i];
				ImGui::TableNextColumn();
				ImGui::Text("%zu", i);
				ImGui::TableNextColumn();
				ImGui::Text("%zu", geoset.vertices.size());
				ImGui::TableNextColumn();
				ImGui::Text("%zu", geoset.faces.size() / 3);
				ImGui::TableNextColumn();
				ImGui::Text("%u", geoset.material_id);
				ImGui::TableNextColumn();
				ImGui::Text("%u", geoset.lod);
				ImGui::TableNextColumn();
				ImGui::Text("%zu", geoset.uv_sets.size());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(geoset.skin.empty() ? "no" : "yes");
				ImGui::TableNextColumn();
				ImGui::Text("%.1f", geoset.extent.bounds_radius);
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();

	place_next_window();
	if (ImGui::Begin("Materials")) {
		for (size_t m = 0; m < model.materials.size(); m++) {
			const auto& material = model.materials[m];
			ImGui::PushID(static_cast<int>(m));
			if (ImGui::CollapsingHeader(std::format("Material {} ({} layers)", m, material.layers.size()).c_str())) {
				ImGui::Text("Priority plane: %u", material.priority_plane);
				ImGui::Text("Flags: %s", material_flags_string(material.flags).c_str());

				for (size_t l = 0; l < material.layers.size(); l++) {
					const auto& layer = material.layers[l];
					ImGui::PushID(static_cast<int>(l));
					if (ImGui::TreeNode(
							"layer",
							"Layer %zu — %s, %s",
							l,
							blend_mode_name(layer.blend_mode),
							shader_type_name(layer.shader)
						)) {
						ImGui::Text("Alpha: %.3f   Coord set: %u", layer.alpha, layer.coord_id);
						ImGui::Text("Shading: %s", layer_shading_flags_string(layer.shading_flags).c_str());
						if (layer.texture_animation_id != static_cast<uint32_t>(-1)) {
							ImGui::Text("Texture animation: %u", layer.texture_animation_id);
						}

						for (const auto& layer_texture : layer.textures) {
							const GLuint tex_id = layer_texture.id < mesh->textures.size() && mesh->textures[layer_texture.id]
								? mesh->textures[layer_texture.id]->id
								: 0;
							ImGui::Image(static_cast<ImTextureID>(tex_id), ImVec2(48, 48));
							ImGui::SameLine();
							const std::string name = layer_texture.id < model.textures.size()
								? texture_display_name(model.textures[layer_texture.id])
								: std::string("(invalid)");
							ImGui::Text("[%u] %s: %s", layer_texture.id, layer_slot_name(layer_texture.slot), name.c_str());
						}

						render_track("Alpha", layer.KMTA);
						render_track("Emissive gain", layer.KMTE);
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}
			ImGui::PopID();
		}
	}
	ImGui::End();

	place_next_window();
	if (ImGui::Begin("Textures")) {
		if (selected_texture >= 0 && selected_texture < static_cast<int>(model.textures.size())) {
			const auto& texture = model.textures[selected_texture];
			const GLuint tex_id = selected_texture < static_cast<int>(mesh->textures.size()) && mesh->textures[selected_texture]
				? mesh->textures[selected_texture]->id
				: 0;
			const auto [tw, th] = texture_size(tex_id);

			ImGui::Text("Texture %d: %s", selected_texture, texture_display_name(texture).c_str());
			ImGui::Text("Replaceable ID: %u", texture.replaceable_id);
			ImGui::Text(
				"Wrap: %s %s",
				(texture.flags & mdx::Texture::wrap_width) ? "width" : "-",
				(texture.flags & mdx::Texture::wrap_height) ? "height" : "-"
			);
			ImGui::Text("Size: %d x %d", tw, th);
			ImGui::SameLine();
			if (ImGui::SmallButton("Close")) {
				selected_texture = -1;
			}
			const float display = std::min(256.0f, ImGui::GetContentRegionAvail().x);
			ImGui::Image(tex_id, ImVec2(display, display));
			ImGui::Separator();
		}

		const ImGuiStyle& style = ImGui::GetStyle();
		constexpr float cell = 96.0f;
		// An ImageButton is the image plus its frame padding on each side.
		const float item_width = cell + style.FramePadding.x * 2.0f;
		const float window_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

		for (size_t i = 0; i < model.textures.size(); i++) {
			ImGui::PushID(static_cast<int>(i));
			ImGui::BeginGroup();

			const GLuint tex_id = i < mesh->textures.size() && mesh->textures[i] ? mesh->textures[i]->id : 0;
			if (ImGui::ImageButton("thumb", tex_id, ImVec2(cell, cell))) {
				selected_texture = static_cast<int>(i);
			}

			const auto [tw, th] = texture_size(tex_id);
			std::string type = model.textures[i].file_name.extension().string();
			const std::string dims = std::format("ID {}, {}x{}, {}", i, tw, th, type);
			ImGui::TextUnformatted(dims.c_str());

			ImGui::EndGroup();

			if (ImGui::IsItemHovered()) {
				if (!type.empty() && type.front() == '.') {
					type.erase(0, 1);
				}
				ImGui::SetTooltip(
					"%s\n%dx%d%s%s",
					texture_display_name(model.textures[i]).c_str(),
					tw,
					th,
					type.empty() ? "" : "  ",
					type.c_str()
				);
			}
			ImGui::PopID();

			// Wrap to the next row only when the following cell would overflow the window width.
			const float next_x2 = ImGui::GetItemRectMax().x + style.ItemSpacing.x + item_width;
			if (i + 1 < model.textures.size() && next_x2 < window_x2) {
				ImGui::SameLine();
			}
		}
	}
	ImGui::End();

	place_next_window();
	if (ImGui::Begin("Sequences")) {
		ImGui::Text("Playing: %s   frame %d", model.sequences[skeleton.sequence_index].name.c_str(), skeleton.current_frame);
		ImGui::Separator();

		if (ImGui::BeginTable("sequences", 7, table_flags | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Start");
			ImGui::TableSetupColumn("End");
			ImGui::TableSetupColumn("Duration");
			ImGui::TableSetupColumn("Loop");
			ImGui::TableSetupColumn("Rarity");
			ImGui::TableSetupColumn("Move speed");
			ImGui::TableHeadersRow();

			for (size_t i = 0; i < model.sequences.size(); i++) {
				const auto& sequence = model.sequences[i];
				ImGui::TableNextColumn();
				if (ImGui::Selectable(sequence.name.c_str(), i == skeleton.sequence_index, ImGuiSelectableFlags_SpanAllColumns)) {
					skeleton.set_sequence(i);
					recenter_camera();
				}
				ImGui::TableNextColumn();
				ImGui::Text("%u", sequence.start_frame);
				ImGui::TableNextColumn();
				ImGui::Text("%u", sequence.end_frame);
				ImGui::TableNextColumn();
				ImGui::Text("%u", sequence.end_frame - sequence.start_frame);
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(sequence.flags == mdx::Sequence::looping ? "yes" : "no");
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", sequence.rarity);
				ImGui::TableNextColumn();
				ImGui::Text("%.1f", sequence.movespeed);
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();

	place_next_window();
	if (ImGui::Begin("Nodes")) {
		std::vector<mdx::Node*> nodes;
		model.for_each_node([&](mdx::Node& node) {
			nodes.push_back(&node);
		});

		std::unordered_set<int> ids;
		for (auto* node : nodes) {
			ids.insert(node->id);
		}

		std::unordered_map<int, std::vector<mdx::Node*>> children;
		std::vector<mdx::Node*> roots;
		for (auto* node : nodes) {
			if (node->parent_id == -1 || !ids.contains(node->parent_id)) {
				roots.push_back(node);
			} else {
				children[node->parent_id].push_back(node);
			}
		}

		std::function<void(mdx::Node*)> draw_node = [&](mdx::Node* node) {
			ImGui::PushID(node->id);
			const auto child_it = children.find(node->id);
			const std::string label =
				std::format("{} [{}] ({})", node->name.empty() ? "<unnamed>" : node->name, node->id, node_type_label(node->flags));
			if (ImGui::TreeNode("node", "%s", label.c_str())) {
				ImGui::TextDisabled("Flags: %s", node_behavior_string(node->flags).c_str());
				render_node_tracks(*node);
				if (child_it != children.end()) {
					for (auto* child : child_it->second) {
						draw_node(child);
					}
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		};

		for (auto* root : roots) {
			draw_node(root);
		}
	}
	ImGui::End();

	place_next_window();
	if (ImGui::Begin("Emitters")) {
		if (!model.emitters1.empty()) {
			ImGui::SeparatorText("Particle Emitters (legacy)");
			ImGui::PushID("emitters1");
			for (size_t i = 0; i < model.emitters1.size(); i++) {
				const auto& emitter = model.emitters1[i];
				ImGui::PushID(static_cast<int>(i));
				if (ImGui::CollapsingHeader(std::format("{}: {}", i, emitter.node.name).c_str())) {
					ImGui::Text("Emission rate: %.3f   Gravity: %.3f", emitter.emission_rate, emitter.gravity);
					ImGui::Text("Longitude: %.3f   Latitude: %.3f", emitter.longitude, emitter.latitude);
					ImGui::Text("Life span: %.3f   Speed: %.3f", emitter.life_span, emitter.speed);
					ImGui::Text("Path: %s", emitter.path.c_str());
					render_node_tracks(emitter.node);
					render_track("Emission rate", emitter.KPEE);
					render_track("Gravity", emitter.KPEG);
					render_track("Visibility", emitter.KPEV);
				}
				ImGui::PopID();
			}
			ImGui::PopID();
		}

		if (!model.emitters2.empty()) {
			ImGui::SeparatorText("Particle Emitters 2");
			ImGui::PushID("emitters2");
			for (size_t i = 0; i < model.emitters2.size(); i++) {
				const auto& emitter = model.emitters2[i];
				ImGui::PushID(static_cast<int>(i));
				if (ImGui::CollapsingHeader(std::format("{}: {}", i, emitter.node.name).c_str())) {
					ImGui::Text("Speed: %.3f   Variation: %.3f", emitter.speed, emitter.speed_variation);
					ImGui::Text("Latitude: %.3f   Gravity: %.3f", emitter.latitude, emitter.gravity);
					ImGui::Text("Life span: %.3f   Emission rate: %.3f", emitter.life_span, emitter.emission_rate);
					ImGui::Text("Size: %.3f x %.3f", emitter.length, emitter.width);
					ImGui::Text("Filter mode: %u   Rows: %u   Columns: %u", emitter.filter_mode, emitter.rows, emitter.columns);
					ImGui::Text("Texture: %u", emitter.texture_id);
					if (emitter.texture_id < mesh->textures.size() && mesh->textures[emitter.texture_id]) {
						ImGui::Image(mesh->textures[emitter.texture_id]->id, ImVec2(64, 64));
					}
					render_node_tracks(emitter.node);
					render_track("Emission rate", emitter.KP2E);
					render_track("Speed", emitter.KP2S);
					render_track("Visibility", emitter.KP2V);
				}
				ImGui::PopID();
			}
			ImGui::PopID();
		}

		if (!model.ribbons.empty()) {
			ImGui::SeparatorText("Ribbon Emitters");
			ImGui::PushID("ribbons");
			for (size_t i = 0; i < model.ribbons.size(); i++) {
				const auto& ribbon = model.ribbons[i];
				ImGui::PushID(static_cast<int>(i));
				if (ImGui::CollapsingHeader(std::format("{}: {}", i, ribbon.node.name).c_str())) {
					ImGui::Text("Height above: %.3f   below: %.3f", ribbon.height_above, ribbon.height_below);
					ImGui::Text("Color: (%.2f, %.2f, %.2f)   Alpha: %.3f", ribbon.color.x, ribbon.color.y, ribbon.color.z, ribbon.alpha);
					ImGui::Text("Life span: %.3f   Emission rate: %u", ribbon.life_span, ribbon.emission_rate);
					ImGui::Text("Material: %u   Rows: %u   Columns: %u", ribbon.material_id, ribbon.rows, ribbon.columns);
					render_node_tracks(ribbon.node);
					render_track("Alpha", ribbon.KRAL);
					render_track("Color", ribbon.KRCO);
					render_track("Visibility", ribbon.KRVS);
				}
				ImGui::PopID();
			}
			ImGui::PopID();
		}

		if (model.emitters1.empty() && model.emitters2.empty() && model.ribbons.empty()) {
			ImGui::TextDisabled("This model has no emitters.");
		}
	}
	ImGui::End();

	place_next_window();
	if (ImGui::Begin("Other")) {
		if (!model.lights.empty()) {
			ImGui::SeparatorText("Lights");
			ImGui::PushID("lights");
			for (size_t i = 0; i < model.lights.size(); i++) {
				const auto& light = model.lights[i];
				ImGui::PushID(static_cast<int>(i));
				if (ImGui::CollapsingHeader(std::format("{}: {}", i, light.node.name).c_str())) {
					ImGui::Text("Type: %d", light.type);
					ImGui::Text("Attenuation: %.1f - %.1f", light.attenuation_start, light.attenuation_end);
					ImGui::Text(
						"Color: (%.2f, %.2f, %.2f)   Intensity: %.2f",
						light.color.x,
						light.color.y,
						light.color.z,
						light.intensity
					);
					ImGui::Text(
						"Ambient: (%.2f, %.2f, %.2f)   Intensity: %.2f",
						light.ambient_color.x,
						light.ambient_color.y,
						light.ambient_color.z,
						light.ambient_intensity
					);
					render_node_tracks(light.node);
				}
				ImGui::PopID();
			}
			ImGui::PopID();
		}

		if (!model.cameras.empty()) {
			ImGui::SeparatorText("Cameras");
			for (size_t i = 0; i < model.cameras.size(); i++) {
				const auto& camera_ = model.cameras[i];
				ImGui::BulletText(
					"%s: pos (%.1f, %.1f, %.1f) -> target (%.1f, %.1f, %.1f), FoV %.1f deg, clip %.1f-%.1f",
					camera_.name.c_str(),
					camera_.position.x,
					camera_.position.y,
					camera_.position.z,
					camera_.target_position.x,
					camera_.target_position.y,
					camera_.target_position.z,
					glm::degrees(camera_.field_of_view),
					camera_.near_clip,
					camera_.far_clip
				);
			}
		}

		if (!model.collision_shapes.empty()) {
			ImGui::SeparatorText("Collision Shapes");
			for (size_t i = 0; i < model.collision_shapes.size(); i++) {
				const auto& shape = model.collision_shapes[i];
				const char* type_name = "Box";
				switch (static_cast<int>(shape.type)) {
					case 0:
						type_name = "Box";
						break;
					case 1:
						type_name = "Plane";
						break;
					case 2:
						type_name = "Sphere";
						break;
					case 3:
						type_name = "Cylinder";
						break;
				}
				ImGui::BulletText("%s: %s   radius %.1f", shape.node.name.c_str(), type_name, shape.radius);
			}
		}

		if (!model.event_objects.empty()) {
			ImGui::SeparatorText("Event Objects");
			for (size_t i = 0; i < model.event_objects.size(); i++) {
				const auto& event = model.event_objects[i];
				ImGui::BulletText("%s: %zu events", event.node.name.c_str(), event.times.size());
			}
		}

		if (!model.texture_animations.empty()) {
			ImGui::SeparatorText("Texture Animations");
			ImGui::PushID("texanims");
			for (size_t i = 0; i < model.texture_animations.size(); i++) {
				const auto& animation = model.texture_animations[i];
				ImGui::PushID(static_cast<int>(i));
				if (ImGui::TreeNode("txan", "Texture animation %zu", i)) {
					render_track("Translation", animation.KTAT);
					render_track("Rotation", animation.KTAR);
					render_track("Scaling", animation.KTAS);
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::PopID();
		}

		if (!model.global_sequences.empty()) {
			ImGui::SeparatorText("Global Sequences");
			for (size_t i = 0; i < model.global_sequences.size(); i++) {
				ImGui::BulletText("%zu: %u ms", i, model.global_sequences[i]);
			}
		}
	}
	ImGui::End();

	place_next_window();
	if (ImGui::Begin(std::format("Validation ({})###Validation", messages.size()).c_str())) {
		ImGui::Checkbox("Errors", &filter_error);
		ImGui::SameLine();
		ImGui::Checkbox("Severe", &filter_severe);
		ImGui::SameLine();
		ImGui::Checkbox("Warnings", &filter_warning);
		ImGui::SameLine();
		ImGui::Checkbox("Unused", &filter_unused);
		ImGui::Separator();

		if (messages.empty()) {
			ImGui::TextDisabled("No issues detected");
		} else {
			size_t shown = 0;
			for (const auto& message : messages) {
				ImVec4 color;
				bool enabled = true;
				switch (message.severity) {
					case mdx::ValidationSeverity::error:
						color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
						enabled = filter_error;
						break;
					case mdx::ValidationSeverity::severe:
						color = ImVec4(1.0f, 0.55f, 0.15f, 1.0f);
						enabled = filter_severe;
						break;
					case mdx::ValidationSeverity::warning:
						color = ImVec4(1.0f, 0.85f, 0.2f, 1.0f);
						enabled = filter_warning;
						break;
					case mdx::ValidationSeverity::unused:
						color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
						enabled = filter_unused;
						break;
				}
				if (!enabled) {
					continue;
				}
				ImGui::TextColored(color, "%s", message.message.c_str());
				shown++;
			}
			if (shown == 0) {
				ImGui::TextDisabled("All %zu messages hidden by filters", messages.size());
			}
		}
	}
	ImGui::End();

	ImGui::Render();
	QtImGui::render(ref);
}
