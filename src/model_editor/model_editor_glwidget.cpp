#include "model_editor_glwidget.h"

#include <QTimer>
#include <QPainter>
#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QFile>
#include <QFileSystemWatcher>
#include <glm/gtx/component_wise.inl>

#include <qt_imgui/qt_imGui.h>

import std;
import OpenGLUtilities;
import BinaryReader;
import Hierarchy;
import MDX;
import Camera;
import ResourceManager;
import <imgui.h>;
import <imgui_internal.h>;

namespace fs = std::filesystem;

// 2025/07/30 These aren't in the class due to some compiler issue with modules
InputHandler my_input_handler;
mdx::MDX::OptimizationStats stats;

ModelEditorGLWidget::ModelEditorGLWidget(
	QWidget* parent,
	const std::shared_ptr<mdx::MDX>& mdx,
	std::vector<mdx::ValidationMessage> messages
) :
	QOpenGLWidget(parent),
	mdx(mdx),
	messages(std::move(messages)) {
	// Give the widget its own persistent native surface. Without this, floating the ADS dock tab
	// reparents the QOpenGLWidget into a new top-level container, tears down/recreates its GL surface,
	// and the viewport renders white. Mirrors the WA_NativeWindow workaround used for the model browser.
	setAttribute(Qt::WA_NativeWindow);
	setAttribute(Qt::WA_DontCreateNativeAncestors);

	makeCurrent();

	setMouseTracking(true);
	setFocus();
	setFocusPolicy(Qt::WheelFocus);

	connect(this, &QOpenGLWidget::frameSwapped, [&]() {
		update();
	});

	// Hot-reload: when the temp .mdl opened by "Edit MDL" changes on disk, queue a reload. Editors
	// often replace the file (drops the watch), so re-add the path and reload on the next paintGL.
	mdl_watcher = new QFileSystemWatcher(this);
	connect(mdl_watcher, &QFileSystemWatcher::fileChanged, [this](const QString& path) {
		reload_pending = true;
		if (!mdl_watcher->files().contains(path) && QFile::exists(path)) {
			mdl_watcher->addPath(path);
		}
	});
}

void ModelEditorGLWidget::initializeGL() {
	ref = QtImGui::initialize(this, false);

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Persist the dock layout in a dedicated file so it survives restarts and doesn't fight over the
	// default imgui.ini in the working directory. The pointer must outlive the context, so keep the
	// string alive for the duration of the program.
	static const std::string ini_path =
		(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/model_editor_imgui.ini").toStdString();
	io.IniFilename = ini_path.c_str();

	// Build the default floating layout only when there is no remembered layout yet.
	build_default_layout = !fs::exists(ini_path);

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
	grid_shader = resource_manager.load<Shader>({"data/shaders/grid.vert", "data/shaders/grid.frag"}).value();

	elapsed_timer.start();

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

	if (reload_pending) {
		reload_pending = false;
		reload_from_mdl();
	}

	skeleton.update_location(glm::vec3(0.f), glm::quat(), glm::vec3(1.f));
	skeleton.update(animation_paused ? 0.0 : delta);

	camera.update(delta);

	glBindVertexArray(vao);
	glClearColor(0.3f, 0.3f, 0.3f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Opaque passes — depth test/write on, blend off (state restored per-layer inside render_opaque)
	glEnable(GL_BLEND);

	shader_sd->use();
	mesh->render_opaque(false, 0, skeleton, camera.projection_view, camera.direction);
	shader_hd->use();
	mesh->render_opaque(true, 0, skeleton, camera.projection_view, camera.direction);

	// Opaque sets depth mask itself, transparent always off
	glDepthMask(false);

	shader_sd->use();
	mesh->render_transparent(false, 0, skeleton, camera.projection_view, camera.direction);
	shader_hd->use();
	mesh->render_transparent(true, 0, skeleton, camera.projection_view, camera.direction);

	glEnable(GL_DEPTH_TEST);

	mesh->render_particles(skeleton, camera.projection_view, camera.X, camera.Y, camera.direction);

	if (draw_grid) {
		render_grid();
	}

	if (draw_extents_box || draw_extents_sphere) {
		render_extents();
	}

	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	render_imgui();
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

	if (ImGui::GetIO().WantCaptureMouse) {
		return;
	}

	camera.mouse_press_event(event);
}

void ModelEditorGLWidget::mouseReleaseEvent(QMouseEvent* event) {
	camera.mouse_release_event(event);
}

void ModelEditorGLWidget::wheelEvent(QWheelEvent* event) {
	if (ImGui::GetIO().WantCaptureMouse) {
		return;
	}

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
	camera.distance = radius / std::sin(glm::radians(camera.fov) * 0.5f);
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

void ModelEditorGLWidget::render_grid() {
	const auto& extent = mesh->mdx->extent;
	float reach = 0.0f;
	if (extent.minimum.x <= extent.maximum.x) {
		reach = std::max({std::abs(extent.minimum.x), std::abs(extent.maximum.x), std::abs(extent.minimum.y), std::abs(extent.maximum.y)});
	}
	const int half = std::clamp(static_cast<int>(std::ceil(std::max(reach, 1.0f) / 128.0f)) * 128, 512, 4096);

	std::vector<glm::vec3> minor;
	std::vector<glm::vec3> major;
	for (int c = -half; c <= half; c += 32) {
		std::vector<glm::vec3>& lines = (c % 128 == 0) ? major : minor;
		lines.emplace_back(static_cast<float>(c), static_cast<float>(-half), 0.0f);
		lines.emplace_back(static_cast<float>(c), static_cast<float>(half), 0.0f);
		lines.emplace_back(static_cast<float>(-half), static_cast<float>(c), 0.0f);
		lines.emplace_back(static_cast<float>(half), static_cast<float>(c), 0.0f);
	}

	grid_shader->use();
	glUniformMatrix4fv(1, 1, GL_FALSE, &camera.projection_view[0][0]);

	glBindVertexArray(line_vao);
	glEnable(GL_DEPTH_TEST);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	if (!minor.empty()) {
		glNamedBufferData(line_vbo, minor.size() * sizeof(glm::vec3), minor.data(), GL_STREAM_DRAW);
		glUniform4f(0, 0.40f, 0.40f, 0.40f, 1.0f);
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(minor.size()));
	}
	if (!major.empty()) {
		glNamedBufferData(line_vbo, major.size() * sizeof(glm::vec3), major.data(), GL_STREAM_DRAW);
		glUniform4f(0, 0.62f, 0.62f, 0.68f, 1.0f);
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(major.size()));
	}
}

void ModelEditorGLWidget::reload_from_mdl() {
	if (hot_reload_path.empty()) {
		return;
	}

	std::ifstream stream(hot_reload_path, std::ios::binary);
	if (!stream) {
		return;
	}
	const std::string data {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};

	auto result = mdx::MDX::from_mdl(data);
	if (!result.has_value()) {
		messages = {{mdx::ValidationSeverity::error, "Hot reload failed to parse MDL: " + result.error()}};
		return;
	}

	auto new_mdx = std::make_shared<mdx::MDX>(std::move(result.value()));

	std::shared_ptr<EditableMesh> new_mesh;
	try {
		new_mesh = std::make_shared<EditableMesh>(new_mdx, std::nullopt);
	} catch (const std::exception& e) {
		messages = new_mdx->validate();
		messages.insert(messages.begin(), {mdx::ValidationSeverity::error, std::string("Hot reload could not build mesh: ") + e.what()});
		return;
	}

	mdx = new_mdx;
	mesh = new_mesh;
	skeleton = SkeletalModelInstance(mdx);
	SkeletalModelInstance::pick_preview_sequence(skeleton, *mdx);
	recenter_camera();
	messages = mdx->validate();
}
