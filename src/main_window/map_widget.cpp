#include "map_widget.h"

#include <QElapsedTimer>
#include <QFont>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <volk.h>

#include <tracy/Tracy.hpp>

// Last, as it imports modules and textual standard library includes can't follow that
#include "render/vulkan_viewport.h"

import std;
import Camera;
import MapGlobal;
import VkResources;
import <glm/glm.hpp>;

/// Renders the loaded map and routes input to the camera and the active brush
class MapViewport : public VulkanViewport {
  public:
	MapViewport() {
		elapsed_timer.start();
	}

	~MapViewport() override {
		wait_idle();
	}

  protected:
	void resizeEvent(QResizeEvent* event) override {
		VulkanViewport::resizeEvent(event);

		delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
		camera.aspect_ratio = static_cast<double>(width()) / height();

		if (!map || !map->loaded) {
			return;
		}
		camera.update(delta);
		map->render_manager.resize_framebuffers(width(), height());
	}

	void record(const VkCommandBuffer cmd, const FrameTarget& target, FrameAllocator& allocator) override {
		delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
		elapsed_timer.start();

		if (map) {
			map->update(delta, width(), height());
		}

		if (map && map->render_debug) {
			paint_debug_overlay();
		} else {
			overlay.clear();
		}
		overlay.upload(cmd, allocator);

		const VkRenderingAttachmentInfo color = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = target.view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {.color = {{0.f, 0.f, 0.f, 1.f}}},
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

		if (map) {
			map->render(cmd, allocator);
		}

		set_viewport(cmd, {{0, 0}, target.extent}, target.extent);
		overlay.draw(cmd);

		vkCmdEndRendering(cmd);

		FrameMark;
	}

	void keyPressEvent(QKeyEvent* event) override {
		if (!map) {
			return;
		}

		input_handler.keys_pressed.emplace(event->key());

		if (map->brush) {
			auto ctx = map->edit_context();
			map->brush->key_press_event(ctx, event);
		}
	}

	void keyReleaseEvent(QKeyEvent* event) override {
		if (!map) {
			return;
		}

		input_handler.keys_pressed.erase(event->key());

		if (map->brush) {
			auto ctx = map->edit_context();
			map->brush->key_release_event(ctx, event);
		}
	}

	void mouseMoveEvent(QMouseEvent* event) override {
		if (!map) {
			return;
		}

		input_handler.mouse_move_event(event);
		camera.mouse_move_event(event);

		if (map->brush) {
			auto ctx = map->edit_context();
			map->brush->mouse_move_event(ctx, event, delta);
		}
	}

	void mousePressEvent(QMouseEvent* event) override {
		if (!map) {
			return;
		}

		camera.mouse_press_event(event);
		if (map->brush) {
			auto ctx = map->edit_context();
			map->brush->mouse_press_event(ctx, event, delta);
		}
	}

	// QWindow, unlike QWidget, does not turn double clicks into press events by default
	void mouseDoubleClickEvent(QMouseEvent* event) override {
		mousePressEvent(event);
	}

	void mouseReleaseEvent(QMouseEvent* event) override {
		if (!map) {
			return;
		}
		camera.mouse_release_event(event);
		if (map->brush) {
			auto ctx = map->edit_context();
			map->brush->mouse_release_event(ctx, event);
		}
	}

	void wheelEvent(QWheelEvent* event) override {
		if (!map) {
			return;
		}

		if (event->modifiers() & Qt::ShiftModifier && map->brush) {
			// Some platforms report the delta on the x-axis when shift is held
			const int delta = event->angleDelta().y() != 0 ? event->angleDelta().y() : event->angleDelta().x();
			if (delta > 0) {
				map->brush->increase_size(1);
			} else if (delta < 0) {
				map->brush->decrease_size(1);
			}
			return;
		}

		camera.mouse_scroll_event(event);
	}

  private:
	QElapsedTimer elapsed_timer;
	double delta = 0.0;
	VulkanOverlay overlay;
	std::vector<double> frametimes;

	void paint_debug_overlay() {
		QImage& image = overlay.begin_paint(size(), devicePixelRatio());
		QPainter p(&image);
		p.setPen(QColor(Qt::GlobalColor::white));
		QFont font("Consolas");
		font.setStyleHint(QFont::Monospace);
		p.setFont(font);

		// Rendering time
		frametimes.push_back(delta);
		if (frametimes.size() > 60) {
			frametimes.erase(frametimes.begin());
		}
		const double average_frametime = std::accumulate(frametimes.begin(), frametimes.end(), 0.0) / frametimes.size();
		p.drawText(10, 20, QString::fromStdString(std::format("Total time: {:.2f}ms", average_frametime * 1000.0)));

		// General info
		auto fmt_vec3 = [](const char* label, glm::vec3 v) {
			return std::format("{:<20} X:{:>6.3f}  Y:{:>6.3f}  Z:{:>6.3f}", label, v.x, v.y, v.z);
		};

		auto fmt_vec2 = [](const char* label, glm::vec2 v) {
			return std::format("{:<20} X:{:>6.3f}  Y:{:>6.3f}", label, v.x, v.y);
		};

		p.drawText(175, 20, QString::fromStdString(fmt_vec3("Mouse World Position", input_handler.mouse_world)));
		p.drawText(175, 35, QString::fromStdString(fmt_vec3("Camera Position", camera.position)));

		if (map->brush) {
			p.drawText(175, 50, QString::fromStdString(fmt_vec2("Brush Grid Position", map->brush->get_position())));
		}

		p.drawText(175, 65, QString::fromStdString(std::format("Camera Horizontal Angle: {:.4f}", camera.horizontal_angle)));
		p.drawText(175, 80, QString::fromStdString(std::format("Camera Vertical Angle: {:.4f}", camera.vertical_angle)));

		auto draw_corner_info = [&](const glm::ivec2 terrain_index, int x, int y) {
			if (terrain_index.x < 0 || terrain_index.y < 0 || terrain_index.x >= map->terrain.width || terrain_index.y >= map->terrain.height) {
				return;
			}

			const auto corner = map->terrain.get_corner(terrain_index.x, terrain_index.y);
			p.drawText(x, y + 20, QString::fromStdString(std::format("Tile info")));
			p.drawText(x, y + 35, QString::fromStdString(std::format("Cliff: {}", corner.cliff)));
			p.drawText(x, y + 50, QString::fromStdString(std::format("Blight: {}", corner.blight)));
			p.drawText(x, y + 65, QString::fromStdString(std::format("Boundary: {}", corner.boundary)));
			p.drawText(x, y + 80, QString::fromStdString(std::format("Cliff texture: {}", corner.cliff_texture)));
			p.drawText(x, y + 95, QString::fromStdString(std::format("Cliff variation: {}", corner.cliff_variation)));
			p.drawText(x, y + 110, QString::fromStdString(std::format("Ground texture: {}", corner.ground_texture)));
			p.drawText(x, y + 125, QString::fromStdString(std::format("Ground variation: {}", corner.ground_variation)));
			p.drawText(x, y + 140, QString::fromStdString(std::format("Height: {}", corner.height)));
			p.drawText(x, y + 155, QString::fromStdString(std::format("Layer height: {}", corner.layer_height)));
			p.drawText(x, y + 170, QString::fromStdString(std::format("Map edge: {}", corner.map_edge)));
			p.drawText(x, y + 185, QString::fromStdString(std::format("Ramp: {}", corner.ramp)));
			p.drawText(x, y + 200, QString::fromStdString(std::format("Romp: {}", corner.romp)));
			p.drawText(x, y + 215, QString::fromStdString(std::format("Special doodad: {}", corner.special_doodad)));
			p.drawText(x, y + 230, QString::fromStdString(std::format("Water: {}", corner.water)));
			p.drawText(x, y + 245, QString::fromStdString(std::format("Water height: {}", corner.water_height)));
		};

		draw_corner_info(glm::ivec2(input_handler.mouse_world), 550, 280);
		draw_corner_info(glm::ivec2(input_handler.mouse_world) + glm::ivec2(1, 0), 750, 280);
		draw_corner_info(glm::ivec2(input_handler.mouse_world) + glm::ivec2(0, 1), 550, 0);
		draw_corner_info(glm::ivec2(input_handler.mouse_world) + glm::ivec2(1, 1), 750, 0);
	}
};

MapWidget::MapWidget(QWidget* parent) : QWidget(parent) {
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	if (!initialize_vulkan()) {
		auto* label = new QLabel("HiveWE needs a GPU with Vulkan 1.3 support", this);
		label->setAlignment(Qt::AlignCenter);
		layout->addWidget(label);
		return;
	}

	// Owned by the container
	auto* viewport = new MapViewport();
	QWidget* container = QWidget::createWindowContainer(viewport, this);
	// Native from the start, so the viewport is embedded in the container's own window. The native minimap makes its
	// siblings native later, and an alien container then leaves the viewport under an empty native window.
	container->setAttribute(Qt::WA_NativeWindow);
	container->setFocusPolicy(Qt::WheelFocus);
	layout->addWidget(container);
}
