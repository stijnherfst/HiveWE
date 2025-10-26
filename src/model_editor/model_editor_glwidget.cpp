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

ModelEditorGLWidget::ModelEditorGLWidget(QWidget* parent, std::shared_ptr<mdx::MDX> mdx) : QOpenGLWidget(parent), mdx(mdx) {
	makeCurrent();

	setMouseTracking(true);
	setFocus();
	setFocusPolicy(Qt::WheelFocus);

	connect(this, &QOpenGLWidget::frameSwapped, [&]() { update(); });
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
	recenter_camera();

	shader = resource_manager.load<Shader>({ "data/shaders/editable_mesh_hd.vert", "data/shaders/editable_mesh_hd.frag" });
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

	shader->use();
	mesh->render(0, skeleton, camera.projection_view, camera.direction);

	glEnable(GL_BLEND);

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

			auto path = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + QString::fromStdString(mesh->mdx->name) + ".mdl";

			std::ofstream file(path.toStdString());
			file.write(mdl.data(), mdl.size());
			file.close();

			QDesktopServices::openUrl(QUrl(path, QUrl::TolerantMode));
		}

		if (ImGui::Button("Open Model")) {
			const QSettings settings;

			const QString file_name = QFileDialog::getOpenFileName(this, "Open Model File",
															 settings.value("openDirectory", QDir::current().path()).toString(),
															 "MDX (*.mdx *.MDX)");

			if (file_name == "") {
				return;
			}

			const fs::path path = file_name.toStdString();
			if (path.extension() != ".mdx" && path.extension() != ".MDX") {
				throw;
			}

			BinaryReader reader = hierarchy.open_file(path).value();
			const auto mdx = std::make_shared<mdx::MDX>(reader);
			mesh = std::make_shared<EditableMesh>(mdx, std::nullopt);
			skeleton = SkeletalModelInstance(mdx);
			recenter_camera();
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
		ImGui::Text(std::format("Looping: {}", mesh->mdx->sequences[skeleton.sequence_index].flags == mdx::Sequence::Flags::looping).c_str());
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
	// Fit mesh extents AABB into screen
	const auto& extent = mesh->mdx->sequences[skeleton.sequence_index].extent;
	const glm::vec3 size = extent.maximum - extent.minimum;
	const float radius = length(size) * 0.5f * 1.1f;
	const float dist = radius / std::sin(camera.fov_rad * 0.5f);
	camera.distance = dist;
	camera.position.z = extent.minimum.z + size.z / 2.f;
}