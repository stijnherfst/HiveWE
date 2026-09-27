#include <QFile>
#include <QFileSystemWatcher>
#include <QLabel>
#include <QStandardPaths>
#include <QWidget>

#include "model_editor_viewport.h"

import std;
import BinaryReader;
import Hierarchy;
import MDX;
import Camera;
import ResourceManager;
import VkContext;

namespace fs = std::filesystem;

// 2025/07/30 These aren't in the class due to some compiler issue with modules
InputHandler my_input_handler;
mdx::MDX::OptimizationStats stats;

/// Mirrors PushConstants in data/shaders/line.*
struct LinePushConstants {
	glm::mat4 mvp;
	glm::vec4 color;
	VkDeviceAddress positions;
};

ModelEditorViewport::ModelEditorViewport(const std::shared_ptr<mdx::MDX>& mdx, std::vector<mdx::ValidationMessage> messages)
	: mdx(mdx),
	  messages(std::move(messages)),
	  line_pipeline({
		  .vertex_shader = "data/shaders/line.vert.spv",
		  .fragment_shader = "data/shaders/line.frag.spv",
		  .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
	  }) {
	// Hot-reload: when the temp .mdl opened by "Edit MDL" changes on disk, queue a reload. Editors
	// often replace the file (drops the watch), so re-add the path and reload on the next frame.
	mdl_watcher = new QFileSystemWatcher(this);
	connect(mdl_watcher, &QFileSystemWatcher::fileChanged, [this](const QString& path) {
		reload_pending = true;
		if (!mdl_watcher->files().contains(path) && QFile::exists(path)) {
			mdl_watcher->addPath(path);
		}
	});

	ref = QtImGui::initialize(this, false);
	QtImGui::setFontUploader(ref, [this](const unsigned char* pixels, const int width, const int height) {
		return imgui_renderer.create_font_texture(pixels, width, height);
	});

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

	try {
		mesh = std::make_shared<VulkanEditableMesh>(mdx, std::nullopt);
		skeleton = Skeleton(mdx);
		Skeleton::pick_preview_sequence(skeleton, *mdx);
		recenter_camera();
	} catch (const std::exception& e) {
		mesh = nullptr;
		this->messages = mdx->validate();
		this->messages.insert(this->messages.begin(), {mdx::ValidationSeverity::error, std::string("Could not build mesh: ") + e.what()});
	}

	elapsed_timer.start();
}

ModelEditorViewport::~ModelEditorViewport() {
	wait_idle();
}

QWidget* ModelEditorViewport::create_widget(const std::shared_ptr<mdx::MDX>& model, std::vector<mdx::ValidationMessage> messages) {
	if (!initialize_vulkan()) {
		auto* label = new QLabel("The model editor needs a GPU with Vulkan 1.3 support");
		label->setAlignment(Qt::AlignCenter);
		return label;
	}
	auto* viewport = new ModelEditorViewport(model, std::move(messages));
	QWidget* container = QWidget::createWindowContainer(viewport);
	viewport->container = container;
	// Keep ADS's own widgets alien; only the viewport needs a native window
	container->setAttribute(Qt::WA_DontCreateNativeAncestors);
	container->setFocusPolicy(Qt::WheelFocus);
	return container;
}

const VulkanTexture* ModelEditorViewport::texture_at(const size_t index) const {
	if (!mesh || index >= mesh->textures.size()) {
		return nullptr;
	}
	return mesh->textures[index].get();
}

void ModelEditorViewport::record(const VkCommandBuffer cmd, const FrameTarget& target, FrameAllocator& allocator) {
	delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	elapsed_timer.start();

	if (reload_pending) {
		reload_pending = false;
		reload_from_mdl();
	}

	skeleton.update_location(glm::vec3(0.f), glm::quat(), glm::vec3(1.f));
	skeleton.update(animation_paused ? 0.0 : delta);

	camera.aspect_ratio = static_cast<float>(target.extent.width) / static_cast<float>(target.extent.height);
	camera.update(delta);

	// Built before recording so the panels see this frame's model state
	render_imgui();

	const VkRenderingAttachmentInfo color = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = target.view,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = {.color = {{0.3f, 0.3f, 0.3f, 1.f}}},
	};
	const VkRenderingAttachmentInfo depth = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = target.depth_view,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.clearValue = {.depthStencil = {1.f, 0}},
	};
	const VkRenderingInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = {{0, 0}, target.extent},
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color,
		.pDepthAttachment = &depth,
	};
	vkCmdBeginRendering(cmd, &rendering);
	bindless.bind(cmd);
	set_viewport(cmd, {{0, 0}, target.extent}, target.extent);

	if (mesh) {
		mesh_renderer.render(
			cmd,
			allocator,
			*mesh,
			skeleton,
			camera.projection_view,
			camera.direction,
			camera.X,
			camera.Y,
			camera.direction,
			0
		);
	}

	if (draw_grid) {
		render_grid(cmd, allocator);
	}

	if (mesh && (draw_extents_box || draw_extents_sphere)) {
		render_extents(cmd, allocator);
	}

	if (show_ui) {
		imgui_renderer.render(cmd, allocator, ImGui::GetDrawData(), target.extent);
	}

	vkCmdEndRendering(cmd);
}

void ModelEditorViewport::keyPressEvent(QKeyEvent* event) {
	my_input_handler.keys_pressed.emplace(event->key());
}

void ModelEditorViewport::keyReleaseEvent(QKeyEvent* event) {
	my_input_handler.keys_pressed.erase(event->key());
}

void ModelEditorViewport::mouseMoveEvent(QMouseEvent* event) {
	my_input_handler.mouse_move_event(event);
	camera.mouse_move_event(event, my_input_handler);
}

void ModelEditorViewport::mousePressEvent(QMouseEvent* event) {
	if (ImGui::GetIO().WantCaptureMouse) {
		return;
	}

	camera.mouse_press_event(event);
}

void ModelEditorViewport::mouseReleaseEvent(QMouseEvent* event) {
	camera.mouse_release_event(event);
}

void ModelEditorViewport::wheelEvent(QWheelEvent* event) {
	if (ImGui::GetIO().WantCaptureMouse) {
		return;
	}

	camera.mouse_scroll_event(event);
}

void ModelEditorViewport::recenter_camera() {
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

void ModelEditorViewport::draw_lines(
	const VkCommandBuffer cmd,
	FrameAllocator& allocator,
	const std::vector<glm::vec3>& lines,
	const glm::vec4 color
) {
	if (lines.empty()) {
		return;
	}
	const auto vertices = allocator.upload(std::span<const glm::vec3>(lines), 16);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, line_pipeline);
	vkCmdSetCullMode(cmd, VK_CULL_MODE_NONE);
	vkCmdSetDepthTestEnable(cmd, VK_TRUE);
	vkCmdSetDepthWriteEnable(cmd, VK_TRUE);
	const LinePushConstants push = {camera.projection_view, color, vertices.address};
	vkCmdPushConstants(cmd, bindless.pipeline_layout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);
	vkCmdDraw(cmd, static_cast<uint32_t>(lines.size()), 1, 0, 0);
}

void ModelEditorViewport::render_extents(const VkCommandBuffer cmd, FrameAllocator& allocator) {
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

	draw_lines(cmd, allocator, lines, glm::vec4(0.f, 1.f, 0.f, 1.f));
}

void ModelEditorViewport::render_grid(const VkCommandBuffer cmd, FrameAllocator& allocator) {
	const auto& extent = mdx->extent;
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

	draw_lines(cmd, allocator, minor, glm::vec4(0.40f, 0.40f, 0.40f, 1.0f));
	draw_lines(cmd, allocator, major, glm::vec4(0.62f, 0.62f, 0.68f, 1.0f));
}

void ModelEditorViewport::reload_from_mdl() {
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

	std::shared_ptr<VulkanEditableMesh> new_mesh;
	try {
		new_mesh = std::make_shared<VulkanEditableMesh>(new_mdx, std::nullopt);
	} catch (const std::exception& e) {
		messages = new_mdx->validate();
		messages.insert(messages.begin(), {mdx::ValidationSeverity::error, std::string("Hot reload could not build mesh: ") + e.what()});
		return;
	}

	mdx = new_mdx;
	// The old mesh's buffers are destroyed once the frames using them have finished
	mesh = new_mesh;
	skeleton = Skeleton(mdx);
	Skeleton::pick_preview_sequence(skeleton, *mdx);
	recenter_camera();
	messages = mdx->validate();
}
