#include "model_grid_glwidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

import std;
import BinaryReader;
import Hierarchy;
import ResourceManager;
import <glm/gtc/matrix_transform.hpp>;

namespace fs = std::filesystem;

namespace {
	constexpr float k_fov_deg = 50.f;
	constexpr float k_near = 0.05f;
	constexpr float k_far = 2000.f;

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
} // namespace

ModelGridGLWidget::ModelGridGLWidget(const std::vector<ModelEntry>& entries, QWidget* parent) : QOpenGLWidget(parent) {
	all_cells.reserve(entries.size());
	for (const auto& e : entries) {
		PreviewCell c;
		c.path = e.path;
		c.category = e.category;
		all_cells.push_back(std::move(c));
	}

	category_mask.set();

	setMouseTracking(false);
	setFocusPolicy(Qt::WheelFocus);

	connect(this, &QOpenGLWidget::frameSwapped, [&]() {
		update();
	});
}

ModelGridGLWidget::~ModelGridGLWidget() {
	makeCurrent();
	all_cells.clear();
	if (vao) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
	doneCurrent();
}

void ModelGridGLWidget::initializeGL() {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	shader_sd = resource_manager.load<Shader>({"data/shaders/editable_mesh_sd.vert", "data/shaders/editable_mesh_sd.frag"}).value();
	shader_hd = resource_manager.load<Shader>({"data/shaders/editable_mesh_hd.vert", "data/shaders/editable_mesh_hd.frag"}).value();

	elapsed_timer.start();
}

void ModelGridGLWidget::resizeGL(const int w, const int h) {
	glViewport(0, 0, w, h);
	const int new_columns = std::max(1, w / cell_size);
	if (new_columns != columns || layout.empty()) {
		columns = new_columns;
		rebuild_layout();
	}
	scroll_offset_y = std::clamp(scroll_offset_y, 0, max_scroll_offset());
	emit_layout_change();
}

void ModelGridGLWidget::paintGL() {
	makeCurrent();

	delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	elapsed_timer.start();

	glDisable(GL_SCISSOR_TEST);
	glViewport(0, 0, width(), height());
	glDepthMask(true);
	glClearColor(0.15f, 0.15f, 0.15f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (columns <= 0 || layout.empty()) {
		return;
	}

	glBindVertexArray(vao);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glDepthFunc(GL_LEQUAL);

	const int view_top = scroll_offset_y;
	const int view_bottom = scroll_offset_y + height();

	for (const auto& row : layout) {
		if (row.kind != LayoutRow::Kind::Cells) {
			continue;
		}
		if (row.y_top + row.height <= view_top || row.y_top >= view_bottom) {
			continue;
		}

		const int row_y_screen = row.y_top - scroll_offset_y;

		for (int c = 0; c < row.visible_count; ++c) {
			const int cell_idx = visible_indices[row.visible_offset + c];
			auto& cell = all_cells[cell_idx];
			if (!cell.loaded && !cell.load_failed) {
				load_cell(cell);
			}
			if (!cell.loaded) {
				continue;
			}

			cell.skeleton.update_location(glm::vec3(0.f), glm::quat(), glm::vec3(1.f));
			cell.skeleton.update(delta);

			const int cell_x_tl = c * cell_size;
			const int cell_y_tl = row_y_screen;

			const int gl_x = cell_x_tl;
			const int gl_y = height() - (cell_y_tl + cell_size);

			glViewport(gl_x, gl_y, cell_size, cell_size);
			glScissor(gl_x, gl_y, cell_size, cell_size);
			glClear(GL_DEPTH_BUFFER_BIT);

			const glm::vec3 dir = glm::normalize(glm::vec3 {-1.f, 1.f, -0.5f});
			const glm::vec3 up = {0, 0, 1};
			const glm::vec3 eye = cell.fit_position - dir * cell.fit_distance;
			const glm::mat4 view = glm::lookAt(eye, cell.fit_position, up);
			const glm::mat4 proj = glm::perspective(glm::radians(k_fov_deg), 1.f, k_near, k_far);
			const glm::mat4 pv = proj * view;

			glEnable(GL_BLEND);
			glDepthMask(true);
			glEnable(GL_DEPTH_TEST);

			shader_sd->use();
			cell.mesh->render_opaque(false, 1, cell.skeleton, pv, dir);
			shader_hd->use();
			cell.mesh->render_opaque(true, 1, cell.skeleton, pv, dir);

			glEnable(GL_BLEND);
			glDepthMask(false);

			shader_sd->use();
			cell.mesh->render_transparent(false, 1, cell.skeleton, pv, dir);
			shader_hd->use();
			cell.mesh->render_transparent(true, 1, cell.skeleton, pv, dir);

			glDepthMask(true);

			const glm::vec3 camera_right = glm::normalize(glm::cross(dir, up));
			const glm::vec3 camera_up = glm::normalize(glm::cross(camera_right, dir));
			cell.mesh->render_particles(cell.skeleton, pv, camera_right, camera_up, dir);
		}
	}

	glDisable(GL_SCISSOR_TEST);
	glBindVertexArray(0);

	QPainter painter(this);
	QFont header_font = painter.font();
	header_font.setBold(true);
	header_font.setPointSizeF(header_font.pointSizeF() + 15.0);
	painter.setFont(header_font);
	const QColor text_color = palette().color(QPalette::WindowText);
	const QColor sep_color = palette().color(QPalette::Mid);

	for (const auto& row : layout) {
		if (row.kind != LayoutRow::Kind::Header) {
			continue;
		}
		if (row.y_top + row.height <= view_top || row.y_top >= view_bottom) {
			continue;
		}
		const int y = row.y_top - scroll_offset_y;
		const QRect r(20, y, width() - 16, row.height);
		painter.setPen(text_color);
		painter.drawText(r, Qt::AlignVCenter | Qt::AlignLeft, QString::fromUtf8(category_label(row.category)));
		painter.setPen(sep_color);
		painter.drawLine(0, y + row.height - 1, width(), y + row.height - 1);
	}
	painter.end();
}

void ModelGridGLWidget::mousePressEvent(QMouseEvent* event) {
	if (event->button() != Qt::LeftButton || columns <= 0) {
		return;
	}
	const int x = event->pos().x();
	const int y = event->pos().y() + scroll_offset_y;

	for (const auto& row : layout) {
		if (y < row.y_top || y >= row.y_top + row.height) {
			continue;
		}
		if (row.kind != LayoutRow::Kind::Cells) {
			return;
		}
		const int col = x / cell_size;
		if (col < 0 || col >= row.visible_count) {
			return;
		}
		const int cell_idx = visible_indices[row.visible_offset + col];
		emit clicked(all_cells[cell_idx].path);
		return;
	}
}

void ModelGridGLWidget::wheelEvent(QWheelEvent* event) {
	const int dy = event->angleDelta().y();
	if (dy == 0) {
		return;
	}
	const int new_offset = std::clamp(scroll_offset_y - dy, 0, max_scroll_offset());
	if (new_offset != scroll_offset_y) {
		scroll_offset_y = new_offset;
		emit scroll_changed(scroll_offset_y);
		update();
	}
	event->accept();
}

void ModelGridGLWidget::set_scroll_offset(int y) {
	const int clamped = std::clamp(y, 0, max_scroll_offset());
	if (clamped != scroll_offset_y) {
		scroll_offset_y = clamped;
		update();
	}
}

void ModelGridGLWidget::set_search(const QString& query) {
	std::string q = lowercase_copy(query.toStdString());
	if (q == search_query) {
		return;
	}
	search_query = std::move(q);
	rebuild_layout();
	scroll_offset_y = std::clamp(scroll_offset_y, 0, max_scroll_offset());
	emit_layout_change();
	update();
}

void ModelGridGLWidget::set_categories(std::bitset<static_cast<size_t>(ModelCategory::Count)> mask) {
	if (mask == category_mask) {
		return;
	}
	category_mask = mask;
	rebuild_layout();
	scroll_offset_y = std::clamp(scroll_offset_y, 0, max_scroll_offset());
	emit_layout_change();
	update();
}

void ModelGridGLWidget::load_cell(PreviewCell& cell) {
	auto reader = hierarchy.open_file(cell.path);
	if (!reader) {
		cell.load_failed = true;
		return;
	}
	try {
		cell.mdx = std::make_shared<mdx::MDX>(reader.value());
		cell.mesh = std::make_shared<EditableMesh>(cell.mdx, std::nullopt);
		cell.skeleton = SkeletalModelInstance(cell.mdx);

		SkeletalModelInstance::pick_preview_sequence(cell.skeleton, *cell.mdx);

		if (cell.mdx->sequences.empty() || cell.skeleton.sequence_index < 0) {
			cell.fit_distance = 200.f;
			cell.fit_position = glm::vec3(0.f);
		} else {
			const auto& extent = cell.mdx->sequences[cell.skeleton.sequence_index].extent;
			const glm::vec3 size = extent.maximum - extent.minimum;
			const float radius = glm::length(size) * 0.5f;
			const float fov_rad = glm::radians(k_fov_deg);
			cell.fit_distance = radius / std::sin(fov_rad * 0.5f);
			cell.fit_position = glm::vec3(0.f, 0.f, extent.minimum.z + size.z * 0.5f);
		}
		cell.loaded = true;
	} catch (...) {
		cell.load_failed = true;
	}
}

void ModelGridGLWidget::rebuild_layout() {
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

int ModelGridGLWidget::content_height_px() const {
	if (layout.empty()) {
		return 0;
	}
	const auto& last = layout.back();
	return last.y_top + last.height;
}

int ModelGridGLWidget::max_scroll_offset() const {
	return std::max(0, content_height_px() - height());
}

void ModelGridGLWidget::emit_layout_change() {
	emit content_height_changed(content_height_px());
	emit scroll_changed(scroll_offset_y);
}
