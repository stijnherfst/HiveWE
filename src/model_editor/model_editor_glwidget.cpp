#include "model_editor_glwidget.h"

#include <QTimer>
#include <QPainter>
#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QDesktopServices>

#include <qt_imgui/qt_imGui.h>

import std;
import OpenGLUtilities;
import BinaryReader;
import Hierarchy;
import MDX;
import Camera;
import ResourceManager;
import <imgui.h>;

namespace fs = std::filesystem;

// 2025/07/30 These aren't in the class due to some compiler issue with modules
InputHandler my_input_handler;
mdx::MDX::OptimizationStats stats;

ModelEditorGLWidget::ModelEditorGLWidget(QWidget* parent, const std::shared_ptr<mdx::MDX>& mdx) : QOpenGLWidget(parent), mdx(mdx) {
	makeCurrent();

	setMouseTracking(true);
	setFocus();
	setFocusPolicy(Qt::WheelFocus);

	connect(this, &QOpenGLWidget::frameSwapped, [&]() {
		update();
	});
}

void ModelEditorGLWidget::initializeGL() {
	ref = QtImGui::initialize(this, false);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0, 0, 0, 1);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	mesh = std::make_shared<EditableMesh>(mdx, std::nullopt);
	skeleton = SkeletalModelInstance(mdx);
	SkeletalModelInstance::pick_preview_sequence(skeleton, *mdx);
	recenter_camera();

	shader_sd = resource_manager.load<Shader>({"data/shaders/editable_mesh_sd.vert", "data/shaders/editable_mesh_sd.frag"}).value();
	shader_hd = resource_manager.load<Shader>({"data/shaders/editable_mesh_hd.vert", "data/shaders/editable_mesh_hd.frag"}).value();

	line_shader = resource_manager.load<Shader>({"data/shaders/physics_debug.vert", "data/shaders/physics_debug.frag"}).value();
	glGenVertexArrays(1, &line_vao);
	glCreateBuffers(1, &line_vbo);
}

void ModelEditorGLWidget::resizeGL(const int w, const int h) {
	glViewport(0, 0, w, h);
	camera.aspect_ratio = double(w) / h;
	camera.update(delta);
	delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
}

void ModelEditorGLWidget::paintGL() {
	makeCurrent();
	
	delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	elapsed_timer.start();

	skeleton.update_location(glm::vec3(0.f), glm::quat(), glm::vec3(1.f));
	skeleton.update(delta);

	camera.update(delta);

	glBindVertexArray(vao);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(true);
	glClearColor(0.3f, 0.3f, 0.3f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Opaque passes — depth test/write on, blend off (state restored per-layer inside render_opaque)
	glEnable(GL_BLEND);
	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);

	shader_sd->use();
	mesh->render_opaque(false, 0, skeleton, camera.projection_view, camera.direction);
	shader_hd->use();
	mesh->render_opaque(true, 0, skeleton, camera.projection_view, camera.direction);

	// Transparent passes — depth write off; depth test/blend func driven per-layer inside render_transparent
	glEnable(GL_BLEND);
	glDepthMask(false);

	shader_sd->use();
	mesh->render_transparent(false, 0, skeleton, camera.projection_view, camera.direction);
	shader_hd->use();
	mesh->render_transparent(true, 0, skeleton, camera.projection_view, camera.direction);

	glDepthMask(true);

	mesh->render_particles(skeleton, camera.projection_view, camera.X, camera.Y, camera.direction);

	if (!draw_extents_box && !draw_extents_sphere) {
		render_extents();
	}

	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	QtImGui::newFrame(ref);

	if (ImGui::Begin("General")) {
		ImGui::Text(std::format("FPS: {:.2f}", 1.0 / delta).c_str());

		if (ImGui::Button("Edit MDL")) {
			auto mdl = mesh->mdx->to_mdl();

			auto path =
				QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + QString::fromStdString(mesh->mdx->name) + ".mdl";

			std::ofstream file(path.toStdString());
			file.write(mdl.data(), mdl.size());
			file.close();

			QDesktopServices::openUrl(QUrl(path, QUrl::TolerantMode));
		}

		if (ImGui::Button("Save to MDX")) {
			const QString file_name = QFileDialog::getSaveFileName(
				this,
				"Save MDX",
				QStandardPaths::writableLocation(QStandardPaths::TempLocation),
				"MDX (*.mdx *.MDX)"
			);

			if (file_name == "") {
				return;
			}

			const fs::path path = file_name.toStdString();

			const auto mdx_data = mdx->to_mdx();
			std::ofstream file(path);
			if (!file) {
				ImGui::OpenPopup("Error");
				ImGui::SetNextWindowSize(ImVec2(400, 100));
				ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::Text("Error saving MDX file");
				ImGui::Text("Could not open file for writing");
				ImGui::EndPopup();
				return;
			}
			file.write(reinterpret_cast<const char*>(mdx_data.buffer.data()), mdx_data.buffer.size());
			file.close();
		}

		ImGui::Text(std::format("name: {}", mesh->mdx->name).c_str());

		size_t vertices = 0;
		size_t triangles = 0;
		for (const auto& i : mesh->mdx->geosets) {
			vertices += i.vertices.size();
			triangles += i.faces.size() / 3;
		}

		ImGui::Text(std::format("Vertices: {}", vertices).c_str());
		ImGui::Text(std::format("Triangles: {}", triangles).c_str());

		const auto& e = mesh->mdx->extent;
		ImGui::Text(
			std::format(
				"Extents:\n\tmin: x {} y {} z {}\n\tmax: x {} y {} z {}",
				e.minimum.x,
				e.minimum.y,
				e.minimum.z,
				e.maximum.x,
				e.maximum.y,
				e.maximum.z
			)
				.c_str()
		);

		if (ImGui::Button("Recalulate Extents")) {
			calculate_animated_extents(mdx);
			recenter_camera();
		}

		ImGui::Text("Draw extents:");
		ImGui::Checkbox("Box", &draw_extents_box);
		ImGui::SameLine();
		ImGui::Checkbox("Sphere", &draw_extents_sphere);
	}
	ImGui::End();

	if (ImGui::Begin("Animation")) {
		ImGui::Text("Animation");
		ImGui::SameLine();
		if (ImGui::BeginCombo("##combo", mesh->mdx->sequences[skeleton.sequence_index].name.c_str())) {
			for (size_t i = 0; i < mesh->mdx->sequences.size(); i++) {
				if (ImGui::Selectable(mesh->mdx->sequences[i].name.c_str(), i == skeleton.sequence_index)) {
					skeleton.set_sequence(i);
				}
				if (i == skeleton.sequence_index) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Text(std::format("Start frame: {}", mesh->mdx->sequences[skeleton.sequence_index].start_frame).c_str());
		ImGui::Text(std::format("End frame: {}", mesh->mdx->sequences[skeleton.sequence_index].end_frame).c_str());
		ImGui::Text(std::format("Current frame: {}", skeleton.current_frame).c_str());
		ImGui::Text(
			std::format("Looping: {}", mesh->mdx->sequences[skeleton.sequence_index].flags == mdx::Sequence::Flags::looping).c_str()
		);
	}
	ImGui::End();

	// if (ImGui::Begin("Optimizer")) {
	// 	static float max_error = 0.0001f;
	// 	if (ImGui::SliderFloat("Max error", &max_error, 0.0f, 0.01f, "%.5f")) {
	// 		BinaryReader reader = hierarchy.open_file("units/human/footman/footman.mdx").value();
	// 		auto mdx = std::make_shared<mdx::MDX>(reader);
	// 		stats = mdx->optimize(max_error);
	//
	// 		const auto writer = mdx->save();
	// 		optimization_file_size_reduction = std::ssize(reader.buffer) - std::ssize(writer.buffer);
	// 		optimization_file_size_reduction_percent = static_cast<float>(std::ssize(writer.buffer) - std::ssize(reader.buffer)) / std::ssize(reader.buffer) * 100.f;
	//
	// 		mesh = std::make_shared<EditableMesh>(mdx, std::nullopt);
	// 		skeleton = SkeletalModelInstance(mesh->mdx);
	// 	}
	// }
	// ImGui::End();
	//
	// if (ImGui::Begin("Optimization Stats")) {
	// 	if (optimization_file_size_reduction >= 0) {
	// 		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Size change: %i KiB (%.2f%%) less", optimization_file_size_reduction / 1024, optimization_file_size_reduction_percent);
	// 	} else {
	// 		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Size change: %i KiB (%.2f%%) more", optimization_file_size_reduction / 1024, optimization_file_size_reduction_percent);
	// 	}
	//
	// 	ImGui::Text("Materials removed %i", stats.materials_removed);
	// 	ImGui::Text("Textures removed %i", stats.textures_removed);
	//
	// 	if (ImGui::BeginTable("Optimization Stats", 3)) {
	// 		ImGui::TableSetupColumn("Type");
	// 		ImGui::TableSetupColumn("Count");
	// 		ImGui::TableSetupColumn("Removed");
	// 		ImGui::TableHeadersRow();
	//
	// 		ImGui::TableNextColumn();
	// 		ImGui::Text("Constant tracks");
	// 		ImGui::TableNextColumn();
	// 		ImGui::Text("%i", stats.constant_tracks);
	// 		ImGui::TableNextColumn();
	// 		{
	// 			const auto percent_removed = stats.constant_tracks_removed > 0 ? static_cast<float>(stats.constant_tracks_removed) / stats.constant_tracks * 100.f : 0.f;
	// 			ImGui::Text("%i (%.2f%%)", stats.constant_tracks_removed, percent_removed);
	// 		}
	//
	// 		ImGui::TableNextColumn();
	// 		ImGui::Text("Linear tracks");
	// 		ImGui::TableNextColumn();
	// 		ImGui::Text("%i", stats.linear_tracks);
	// 		ImGui::TableNextColumn();
	// 		{
	// 			const auto percent_removed = stats.linear_tracks_removed > 0 ? static_cast<float>(stats.linear_tracks_removed) / stats.linear_tracks * 100.f : 0.f;
	// 			ImGui::Text("%i (%.2f%%)", stats.linear_tracks_removed, percent_removed);
	// 		}
	//
	// 		ImGui::TableNextColumn();
	// 		ImGui::Text("Bezier tracks");
	// 		ImGui::TableNextColumn();
	// 		ImGui::Text("%i", stats.bezier_tracks);
	// 		ImGui::TableNextColumn();
	// 		{
	// 			const auto percent_removed = stats.bezier_tracks_removed > 0 ? static_cast<float>(stats.bezier_tracks_removed) / stats.bezier_tracks * 100.f : 0.f;
	// 			ImGui::Text("%i (%.2f%%)", stats.bezier_tracks_removed, percent_removed);
	// 		}
	//
	// 		ImGui::TableNextColumn();
	// 		ImGui::Text("Hermite tracks");
	// 		ImGui::TableNextColumn();
	// 		ImGui::Text("%i", stats.hermite_tracks);
	// 		ImGui::TableNextColumn();
	// 		{
	// 			const auto percent_removed = stats.hermite_tracks_removed > 0 ? static_cast<float>(stats.hermite_tracks_removed) / stats.hermite_tracks * 100.f : 0.f;
	// 			ImGui::Text("%i (%.2f%%)", stats.hermite_tracks_removed, percent_removed);
	// 		}
	//
	// 		ImGui::EndTable();
	// 	}
	// }
	// ImGui::End();

	ImGui::Render();
	QtImGui::render(ref);
}

void ModelEditorGLWidget::keyPressEvent(QKeyEvent* event) {
	my_input_handler.keys_pressed.emplace(event->key());
}

void ModelEditorGLWidget::keyReleaseEvent(QKeyEvent* event) {
	my_input_handler.keys_pressed.erase(event->key());
}

void ModelEditorGLWidget::mouseMoveEvent(QMouseEvent* event) {
	my_input_handler.mouse_move_event(event);
	camera.mouse_move_event(event, my_input_handler);
}

void ModelEditorGLWidget::mousePressEvent(QMouseEvent* event) {
	makeCurrent();

	camera.mouse_press_event(event);
}

void ModelEditorGLWidget::mouseReleaseEvent(QMouseEvent* event) {
	camera.mouse_release_event(event);
}

void ModelEditorGLWidget::wheelEvent(QWheelEvent* event) {
	camera.mouse_scroll_event(event);
}

void ModelEditorGLWidget::recenter_camera() {
	// Fit mesh extents AABB into screen. Some sequences (spell missiles, etc.) ship with the
	// sentinel extent (min = +FLT_MAX, max = -FLT_MAX); using that directly would compute
	// length() as +inf and push the camera to infinity. Fall back to bounds_radius, or a
	// hardcoded default if that's also missing.
	const auto& extent = mesh->mdx->sequences[skeleton.sequence_index].extent;
	float radius;
	if (extent.minimum.x <= extent.maximum.x) {
		const glm::vec3 size = extent.maximum - extent.minimum;
		radius = length(size) * 0.5f * 1.1f;
		camera.position.z = extent.minimum.z + size.z / 2.f;
	} else {
		radius = (extent.bounds_radius > 0.f ? extent.bounds_radius : 200.f) * 1.1f;
		camera.position.z = 0.f;
	}
	camera.distance = radius / std::sin(camera.fov_rad * 0.5f);
}

void ModelEditorGLWidget::render_extents() {
	const auto& extent = mesh->mdx->sequences[skeleton.sequence_index].extent;

	std::vector<glm::vec3> lines;

	// AABB: 12 edges between the 8 corners. Skip the sentinel/empty extent (min > max).
	if (draw_extents_box && extent.minimum.x <= extent.maximum.x) {
		const glm::vec3 mn = extent.minimum;
		const glm::vec3 mx = extent.maximum;

		const glm::vec3 corner[8] = {
			{mn.x, mn.y, mn.z},
			{mx.x, mn.y, mn.z},
			{mx.x, mx.y, mn.z},
			{mn.x, mx.y, mn.z},
			{mn.x, mn.y, mx.z},
			{mx.x, mn.y, mx.z},
			{mx.x, mx.y, mx.z},
			{mn.x, mx.y, mx.z},
		};
		const int edge[12][2] = {
			{0, 1},
			{1, 2},
			{2, 3},
			{3, 0}, // bottom
			{4, 5},
			{5, 6},
			{6, 7},
			{7, 4}, // top
			{0, 4},
			{1, 5},
			{2, 6},
			{3, 7}, // verticals
		};
		for (const auto& e : edge) {
			lines.push_back(corner[e[0]]);
			lines.push_back(corner[e[1]]);
		}
	}

	// Bounding sphere: three great circles (XY, XZ, YZ) centered on the AABB center.
	if (draw_extents_sphere && extent.bounds_radius > 0.f && extent.minimum.x <= extent.maximum.x) {
		const glm::vec3 c = (extent.minimum + extent.maximum) * 0.5f;
		const float r = extent.bounds_radius;
		constexpr int segments = 48;
		constexpr float two_pi = 6.283185307f;
		for (int i = 0; i < segments; i++) {
			const float a0 = two_pi * i / segments;
			const float a1 = two_pi * (i + 1) / segments;
			const glm::vec2 p0(std::cos(a0) * r, std::sin(a0) * r);
			const glm::vec2 p1(std::cos(a1) * r, std::sin(a1) * r);

			lines.emplace_back(c + glm::vec3(p0.x, p0.y, 0.f));
			lines.emplace_back(c + glm::vec3(p1.x, p1.y, 0.f)); // XY
			lines.emplace_back(c + glm::vec3(p0.x, 0.f, p0.y));
			lines.emplace_back(c + glm::vec3(p1.x, 0.f, p1.y)); // XZ
			lines.emplace_back(c + glm::vec3(0.f, p0.x, p0.y));
			lines.emplace_back(c + glm::vec3(0.f, p1.x, p1.y)); // YZ
		}
	}

	if (lines.empty()) {
		return;
	}

	glNamedBufferData(line_vbo, lines.size() * sizeof(glm::vec3), lines.data(), GL_STREAM_DRAW);

	line_shader->use();
	glUniformMatrix4fv(1, 1, GL_FALSE, &camera.projection_view[0][0]);

	glBindVertexArray(line_vao);
	glEnable(GL_DEPTH_TEST);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size()));
}
