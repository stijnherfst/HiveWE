#include "model_grid_widget.h"

#include <QElapsedTimer>
#include <QGuiApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <volk.h>

#include "render/vulkan_viewport.h"

import std;
import BinaryReader;
import Hierarchy;
import MDX;
import Skeleton;
import VkEditableMesh;
import VkResources;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;

namespace fs = std::filesystem;

namespace {
	constexpr float k_fov_deg = 50.f;
	constexpr float k_near = 0.1f;
	constexpr float k_far = 20'000.f;

	const char* category_label(const ModelCategory c) {
		switch (c) {
			case ModelCategory::Abilities:
				return "Abilities";
			case ModelCategory::Buildings:
				return "Buildings";
			case ModelCategory::Doodads:
				return "Doodads";
			case ModelCategory::Environment:
				return "Environment";
			case ModelCategory::Objects:
				return "Objects";
			case ModelCategory::SharedModels:
				return "Shared Models";
			case ModelCategory::Units:
				return "Units";
			case ModelCategory::Map:
				return "Map";
			default:
				return "";
		}
	}

	std::string lowercase_copy(std::string s) {
		for (char& c : s) {
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		return s;
	}

	const char* severity_prefix(const mdx::ValidationSeverity severity) {
		switch (severity) {
			case mdx::ValidationSeverity::error:
				return "Error: ";
			case mdx::ValidationSeverity::severe:
				return "Severe: ";
			case mdx::ValidationSeverity::warning:
				return "Warning: ";
			case mdx::ValidationSeverity::unused:
				return "Unused: ";
		}
		return "";
	}
} // namespace

/// The Vulkan window behind ModelGridWidget. Holds the grid state; the widget forwards its API here.
class ModelGridViewport : public VulkanViewport {
  public:
	ModelGridViewport(ModelGridWidget* owner, const std::vector<ModelEntry>& entries, const bool single_preview)
		: owner(owner),
		  single_preview(single_preview) {
		all_cells.reserve(entries.size());
		for (const auto& e : entries) {
			PreviewCell c;
			c.path = e.path;
			c.category = e.category;
			all_cells.push_back(std::move(c));
		}
		category_mask.set();
		elapsed_timer.start();
	}

	~ModelGridViewport() override {
		wait_idle();
	}

	int cell_size = 128;

	void set_empty_message(const QString& message) {
		empty_message = message;
		overlay_dirty = true;
	}

	void set_scroll_offset(const int y) {
		const int clamped = std::clamp(y, 0, max_scroll_offset());
		if (clamped != scroll_offset_y) {
			scroll_offset_y = clamped;
			overlay_dirty = true;
		}
	}

	void set_search(const QString& query) {
		std::string q = lowercase_copy(query.toStdString());
		if (q == search_query) {
			return;
		}
		search_query = std::move(q);
		relayout();
	}

	void set_categories(const std::bitset<static_cast<size_t>(ModelCategory::Count)> mask) {
		if (mask == category_mask) {
			return;
		}
		category_mask = mask;
		relayout();
	}

  protected:
	void resizeEvent(QResizeEvent* event) override {
		VulkanViewport::resizeEvent(event);
		overlay_dirty = true;
		if (single_preview) {
			return;
		}
		const int new_columns = std::max(1, width() / cell_size);
		if (new_columns != columns || layout.empty()) {
			columns = new_columns;
			rebuild_layout();
		}
		scroll_offset_y = std::clamp(scroll_offset_y, 0, max_scroll_offset());
		emit_layout_change();
	}

	void mousePressEvent(QMouseEvent* event) override {
		if (event->button() != Qt::LeftButton) {
			return;
		}
		if (const PreviewCell* cell = cell_at(event->position().toPoint())) {
			emit owner->clicked(cell->path);
			if (event->type() == QEvent::MouseButtonDblClick) {
				emit owner->double_clicked(cell->path);
			}
		}
	}

	// QWindow, unlike QWidget, does not turn double clicks into press events by default
	void mouseDoubleClickEvent(QMouseEvent* event) override {
		mousePressEvent(event);
	}

	// QWindow gets no QEvent::ToolTip, so the load error tooltip follows the mouse instead
	void mouseMoveEvent(QMouseEvent* event) override {
		const PreviewCell* cell = cell_at(event->position().toPoint());
		if (cell && cell->load_failed && !cell->load_error_message.empty()) {
			QToolTip::showText(event->globalPosition().toPoint(), QString::fromStdString(cell->load_error_message));
		} else {
			QToolTip::hideText();
		}
	}

	void wheelEvent(QWheelEvent* event) override {
		const int dy = event->angleDelta().y();
		if (dy == 0 || single_preview) {
			return;
		}
		const int new_offset = std::clamp(scroll_offset_y - dy, 0, max_scroll_offset());
		if (new_offset != scroll_offset_y) {
			scroll_offset_y = new_offset;
			overlay_dirty = true;
			emit owner->scroll_changed(scroll_offset_y);
		}
		event->accept();
	}

	void record(const VkCommandBuffer cmd, const FrameTarget& target, FrameAllocator& allocator) override {
		delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
		elapsed_timer.start();

		// Loading happens before painting the overlay, since a cell that fails to load gets an error label
		const std::vector<std::pair<PreviewCell*, QRect>> cells = visible_cells();
		for (const auto& [cell, rect] : cells) {
			if (!cell->loaded && !cell->load_failed) {
				load_cell(*cell);
				overlay_dirty |= cell->load_failed;
			}
		}

		if (overlay_dirty) {
			paint_overlay();
			overlay_dirty = false;
		}
		overlay.upload(cmd, allocator);

		const VkRenderingAttachmentInfo color = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = target.view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {.color = {{0.15f, 0.15f, 0.15f, 1.f}}},
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

		const qreal ratio = devicePixelRatio();
		for (const auto& [cell, rect] : cells) {
			if (!cell->loaded) {
				continue;
			}
			const VkRect2D pixels = {
				{static_cast<int32_t>(std::lround(rect.x() * ratio)), static_cast<int32_t>(std::lround(rect.y() * ratio))},
				{static_cast<uint32_t>(std::lround(rect.width() * ratio)), static_cast<uint32_t>(std::lround(rect.height() * ratio))},
			};
			const VkRect2D scissor = set_viewport(cmd, pixels, target.extent);
			if (scissor.extent.width == 0 || scissor.extent.height == 0) {
				continue;
			}

			// Each cell gets its own depth range, so models never occlude their neighbours
			if (!single_preview) {
				const VkClearAttachment clear = {
					.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
					.clearValue = {.depthStencil = {1.f, 0}},
				};
				const VkClearRect clear_rect = {scissor, 0, 1};
				vkCmdClearAttachments(cmd, 1, &clear, 1, &clear_rect);
			}

			const float aspect = rect.height() > 0 ? static_cast<float>(rect.width()) / static_cast<float>(rect.height()) : 1.f;
			render_cell(cmd, allocator, *cell, aspect);
		}

		// The overlay covers the whole window
		set_viewport(cmd, {{0, 0}, target.extent}, target.extent);
		overlay.draw(cmd);

		vkCmdEndRendering(cmd);
	}

  private:
	struct PreviewCell {
		fs::path path;
		ModelCategory category;
		std::shared_ptr<mdx::MDX> mdx;
		std::shared_ptr<VulkanEditableMesh> mesh;
		Skeleton skeleton;
		float fit_radius = 0.f; // bounding-sphere radius, used to fit the model for any viewport aspect
		glm::vec3 fit_position {0.f, 0.f, 0.f};
		bool loaded = false;
		bool load_failed = false;
		std::string load_error_message;
	};

	struct LayoutRow {
		enum class Kind {
			Header,
			Cells
		};
		Kind kind;
		ModelCategory category;
		int visible_offset = 0;
		int visible_count = 0;
		int y_top = 0;
		int height = 0;
	};

	ModelGridWidget* owner;
	std::vector<PreviewCell> all_cells;
	std::vector<int> visible_indices;
	std::vector<LayoutRow> layout;

	EditableMeshRenderer renderer;
	VulkanOverlay overlay;
	bool overlay_dirty = true;

	int header_height = 40;
	int columns = 1;
	int scroll_offset_y = 0;
	bool single_preview = false;
	QString empty_message;

	std::string search_query;
	std::bitset<static_cast<size_t>(ModelCategory::Count)> category_mask;

	QElapsedTimer elapsed_timer;
	double delta = 0.0;

	/// The cells that intersect the window, with their rectangles in logical window coordinates
	std::vector<std::pair<PreviewCell*, QRect>> visible_cells() {
		std::vector<std::pair<PreviewCell*, QRect>> cells;
		if (single_preview) {
			if (!all_cells.empty()) {
				cells.emplace_back(&all_cells[0], QRect(0, 0, width(), height()));
			}
			return cells;
		}

		const int view_top = scroll_offset_y;
		const int view_bottom = scroll_offset_y + height();
		for (const auto& row : layout) {
			if (row.kind != LayoutRow::Kind::Cells || row.y_top + row.height <= view_top || row.y_top >= view_bottom) {
				continue;
			}
			for (int c = 0; c < row.visible_count; ++c) {
				const int cell_idx = visible_indices[row.visible_offset + c];
				cells.emplace_back(&all_cells[cell_idx], QRect(c * cell_size, row.y_top - scroll_offset_y, cell_size, cell_size));
			}
		}
		return cells;
	}

	/// The grid cell under a position in window coordinates. Single previews have no cells to click.
	const PreviewCell* cell_at(const QPoint position) const {
		if (single_preview || columns <= 0) {
			return nullptr;
		}
		const int y = position.y() + scroll_offset_y;
		for (const auto& row : layout) {
			if (y < row.y_top || y >= row.y_top + row.height) {
				continue;
			}
			if (row.kind != LayoutRow::Kind::Cells) {
				return nullptr;
			}
			const int col = position.x() / cell_size;
			if (col < 0 || col >= row.visible_count) {
				return nullptr;
			}
			return &all_cells[visible_indices[row.visible_offset + col]];
		}
		return nullptr;
	}

	void paint_overlay() {
		QImage& image = overlay.begin_paint(size(), devicePixelRatio());
		QPainter painter(&image);
		const QPalette palette = QGuiApplication::palette();
		const QRect rect(0, 0, width(), height());

		if (single_preview) {
			if (all_cells.empty()) {
				if (!empty_message.isEmpty()) {
					painter.setPen(palette.color(QPalette::WindowText));
					painter.drawText(rect.adjusted(10, 10, -10, -10), Qt::AlignCenter | Qt::TextWordWrap, empty_message);
				}
			} else if (all_cells[0].load_failed) {
				const auto& cell = all_cells[0];
				painter.setPen(palette.color(QPalette::WindowText));
				const QString message = cell.load_error_message.empty() ? QString("Error") : QString::fromStdString(cell.load_error_message);
				painter.drawText(rect.adjusted(10, 10, -10, -10), Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, message);
			}
			return;
		}

		QFont header_font = QGuiApplication::font();
		header_font.setBold(true);
		header_font.setPointSizeF(header_font.pointSizeF() + 15.0);
		painter.setFont(header_font);
		constexpr QColor warning_text_color = QColorConstants::DarkRed;
		const QColor text_color = palette.color(QPalette::WindowText);
		const QColor sep_color = palette.color(QPalette::Mid);

		const int view_top = scroll_offset_y;
		const int view_bottom = scroll_offset_y + height();
		for (const auto& row : layout) {
			if (row.y_top + row.height <= view_top || row.y_top >= view_bottom) {
				continue;
			}
			const int row_y_screen = row.y_top - scroll_offset_y;

			if (row.kind == LayoutRow::Kind::Header) {
				const QRect r(20, row_y_screen, width() - 16, row.height);
				painter.setPen(text_color);
				painter.drawText(r, Qt::AlignVCenter | Qt::AlignLeft, QString::fromUtf8(category_label(row.category)));
				painter.setPen(sep_color);
				painter.drawLine(0, row_y_screen + row.height - 1, width(), row_y_screen + row.height - 1);
				continue;
			}

			for (int c = 0; c < row.visible_count; ++c) {
				const int cell_idx = visible_indices[row.visible_offset + c];
				if (!all_cells[cell_idx].load_failed) {
					continue;
				}
				painter.setPen(warning_text_color);
				painter.drawText(QRect {c * cell_size, row_y_screen, cell_size, cell_size}, Qt::AlignCenter, "Error");
			}
		}
	}

	void load_cell(PreviewCell& cell) const {
		const auto reader = hierarchy.open_file(cell.path, Hierarchy::FileSource::all, {".mdx", ".mdl"});
		if (!reader) {
			cell.load_failed = true;
			cell.load_error_message = reader.error();
			return;
		}

		auto file = reader.value();

		try {
			if (mdx::is_mdx(file)) {
				cell.mdx = std::make_shared<mdx::MDX>(file);
			} else {
				const auto view = std::string_view(reinterpret_cast<const char*>(file.buffer.data()), file.buffer.size());
				const auto result = mdx::MDX::from_mdl(view);

				if (!result) {
					cell.load_failed = true;
					cell.load_error_message = result.error();
					return;
				}

				cell.mdx = std::make_shared<mdx::MDX>(std::move(result.value()));
			}

			if (!cell.mdx->is_valid()) {
				// The model has severe errors and cannot be rendered; report why so the user can act on it.
				std::string message = "This model cannot be rendered:";
				for (const auto& validation : cell.mdx->validate()) {
					message += '\n';
					message += severity_prefix(validation.severity);
					message += validation.message;
				}
				cell.load_failed = true;
				cell.load_error_message = std::move(message);
				return;
			}

			cell.mesh = std::make_shared<VulkanEditableMesh>(cell.mdx, std::nullopt);
			cell.skeleton = Skeleton(cell.mdx);

			Skeleton::pick_preview_sequence(cell.skeleton, *cell.mdx);

			const auto& extent =
				cell.skeleton.sequence_index == -1 ? cell.mdx->extent : cell.mdx->sequences[cell.skeleton.sequence_index].extent;
			const glm::vec3 size = extent.maximum - extent.minimum;
			cell.fit_position = glm::vec3(0.f, 0.f, extent.minimum.z + size.z * 0.5f);
			cell.fit_radius = glm::length(size) * 0.5f;
			cell.loaded = true;
		} catch (std::exception& e) {
			cell.load_failed = true;
			cell.load_error_message = e.what();
		}
	}

	/// Renders one model into the current viewport; aspect is the viewport width / height.
	void render_cell(const VkCommandBuffer cmd, FrameAllocator& allocator, PreviewCell& cell, const float aspect) {
		cell.skeleton.update_location(glm::vec3(0.f), glm::quat(), glm::vec3(1.f));
		cell.skeleton.update(delta);

		const glm::vec3 dir = glm::normalize(glm::vec3 {-1.f, 1.f, -0.5f});
		constexpr glm::vec3 up = {0, 0, 1};
		// Pull the camera back far enough that the bounding sphere fits the tighter of the vertical
		// and horizontal field of view, so the model stays fully framed at any viewport aspect ratio.
		constexpr float half_fov = glm::radians(k_fov_deg) * 0.5f;
		const float horizontal_half_fov = std::atan(std::tan(half_fov) * aspect);
		const float limiting_half_fov = std::min(half_fov, horizontal_half_fov);
		const float fit_distance = cell.fit_radius / std::sin(limiting_half_fov);
		const glm::vec3 eye = cell.fit_position - dir * fit_distance;
		const glm::mat4 view = glm::lookAt(eye, cell.fit_position, up);
		const glm::mat4 projection = glm::perspective(glm::radians(k_fov_deg), aspect, k_near, k_far);
		const glm::mat4 projection_view = projection * view;

		const glm::vec3 camera_right = glm::normalize(glm::cross(dir, up));
		const glm::vec3 camera_up = glm::normalize(glm::cross(camera_right, dir));
		renderer.render(cmd, allocator, *cell.mesh, cell.skeleton, projection_view, dir, camera_right, camera_up, dir, 1);
	}

	void relayout() {
		rebuild_layout();
		scroll_offset_y = std::clamp(scroll_offset_y, 0, max_scroll_offset());
		overlay_dirty = true;
		emit_layout_change();
	}

	void rebuild_layout() {
		layout.clear();
		visible_indices.clear();
		if (columns <= 0) {
			return;
		}

		std::vector<std::vector<int>> by_category(static_cast<size_t>(ModelCategory::Count));
		for (int i = 0; i < static_cast<int>(all_cells.size()); ++i) {
			const auto& cell = all_cells[i];
			const size_t ci = static_cast<size_t>(cell.category);
			if (!category_mask.test(ci)) {
				continue;
			}
			if (!search_query.empty()) {
				const std::string stem = lowercase_copy(cell.path.stem().string());
				if (stem.find(search_query) == std::string::npos) {
					continue;
				}
			}
			by_category[ci].push_back(i);
		}

		int y_cursor = 0;
		for (size_t ci = 0; ci < static_cast<size_t>(ModelCategory::Count); ++ci) {
			auto& bucket = by_category[ci];
			if (bucket.empty()) {
				continue;
			}

			LayoutRow header;
			header.kind = LayoutRow::Kind::Header;
			header.category = static_cast<ModelCategory>(ci);
			header.y_top = y_cursor;
			header.height = header_height;
			layout.push_back(header);
			y_cursor += header_height;

			const int offset = static_cast<int>(visible_indices.size());
			visible_indices.insert(visible_indices.end(), bucket.begin(), bucket.end());

			const int total = static_cast<int>(bucket.size());
			int remaining = total;
			int row_idx = 0;
			while (remaining > 0) {
				const int this_row = std::min(remaining, columns);
				LayoutRow r;
				r.kind = LayoutRow::Kind::Cells;
				r.category = static_cast<ModelCategory>(ci);
				r.visible_offset = offset + row_idx * columns;
				r.visible_count = this_row;
				r.y_top = y_cursor;
				r.height = cell_size;
				layout.push_back(r);
				y_cursor += cell_size;
				remaining -= this_row;
				++row_idx;
			}
		}
	}

	int content_height_px() const {
		if (layout.empty()) {
			return 0;
		}
		const auto& last = layout.back();
		return last.y_top + last.height;
	}

	int max_scroll_offset() const {
		return std::max(0, content_height_px() - height());
	}

	void emit_layout_change() {
		emit owner->content_height_changed(content_height_px());
		emit owner->scroll_changed(scroll_offset_y);
	}
};

ModelGridWidget::ModelGridWidget(const std::vector<ModelEntry>& entries, QWidget* parent, const bool single_preview)
	: QWidget(parent) {
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	if (!initialize_vulkan()) {
		auto* label = new QLabel("Model previews need a GPU with Vulkan 1.3 support", this);
		label->setAlignment(Qt::AlignCenter);
		layout->addWidget(label);
		return;
	}

	viewport = new ModelGridViewport(this, entries, single_preview);
	QWidget* container = QWidget::createWindowContainer(viewport, this);
	// Keep ADS's own widgets alien; only the viewport needs a native window
	container->setAttribute(Qt::WA_DontCreateNativeAncestors);
	container->setFocusPolicy(Qt::WheelFocus);
	layout->addWidget(container);
}

int ModelGridWidget::cell_pixel_size() const {
	return viewport ? viewport->cell_size : 128;
}

void ModelGridWidget::set_empty_message(const QString& message) {
	if (viewport) {
		viewport->set_empty_message(message);
	}
}

void ModelGridWidget::set_scroll_offset(const int y) {
	if (viewport) {
		viewport->set_scroll_offset(y);
	}
}

void ModelGridWidget::set_search(const QString& query) {
	if (viewport) {
		viewport->set_search(query);
	}
}

void ModelGridWidget::set_categories(const std::bitset<static_cast<size_t>(ModelCategory::Count)> mask) {
	if (viewport) {
		viewport->set_categories(mask);
	}
}
