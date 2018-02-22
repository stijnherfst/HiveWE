#include "stdafx.h"

void Brush::create() {
	//for (int i = 0; i < size * 2 + 1; i++) {
	//	for (int j = 0; j < size * 2 + 1; j++) {
	//		if (std::sqrt(i * i + j * j) < size * 0.5) {
	//			brush[j * (size * 2 + 1) + i] = { 0, 255, 0, 255 };
	//		} else {
	//			brush[j * (size * 2 + 1) + i] = { 0, 0, 0, 0 };
	//		}
	//	}
	//}

	int cells = std::ceil((this->size * 2 + 1 + 3) / 4.f);
	brush.resize(cells * 4 * cells * 4, { 0, 0, 0, 0 });

	for (int i = 0; i < this->size * 2 + 1; i++) {
		for (int j = 0; j < this->size * 2 + 1; j++) {
			brush[j * cells * 4 + i] = { 0, 255, 0, 128 };
		}
	}

	gl->glGenTextures( 1, &brush_texture );
	gl->glBindTexture( GL_TEXTURE_2D, brush_texture );
	gl->glBindTexture(GL_TEXTURE_2D, brush_texture);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, cells * 4, cells * 4, 0, GL_BGRA, GL_UNSIGNED_BYTE, brush.data());
	gl->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	gl->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	shader = resource_manager.load<Shader>({ "Data/Shaders/brush.vs", "Data/Shaders/brush.fs" });
}

void Brush::set_position(const glm::vec2 position) {
	glm::vec2 center_position = position - size * 0.25f;
	this->position = glm::floor(center_position);
	uv_offset = glm::abs((center_position - glm::vec2(this->position)) * 4.f);
}

void Brush::set_size(const int size) {
	int change = size - this->size;

	this->size = std::clamp(size, 0, 240);
	int cells = std::ceil((this->size * 2 + 1 + 3) / 4.f);
	brush.clear();
	brush.resize(cells * 4 * cells * 4, { 0, 0, 0, 0 });

	for (int i = 0; i < this->size * 2 + 1; i++) {
		for (int j = 0; j < this->size * 2 + 1; j++) {
			brush[j * cells * 4 + i] = { 0, 255, 0, 128 };
		}
	}

	gl->glBindTexture(GL_TEXTURE_2D, brush_texture);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, cells * 4, cells * 4, 0, GL_BGRA, GL_UNSIGNED_BYTE, brush.data());

	set_position(glm::vec2(position) + glm::vec2(uv_offset + size - change) * 0.25f);
}

void Brush::render(Terrain& terrain) {
	glm::ivec2 center_position = position + (size + uv_offset) / 4;
	if (center_position.x < 0 || center_position.y < 0 || center_position.x > terrain.width || center_position.y > terrain.height) {
		return;
	}

	gl->glDisable(GL_DEPTH_TEST);

	shader->use();

	// +3 for uv_offset so it can move sub terrain cell
	int cells = std::ceil((this->size * 2 + 1 + 3) / 4.f);

	gl->glUniformMatrix4fv(1, 1, GL_FALSE, &camera.projection_view[0][0]);
	gl->glUniform2f(2, position.x, position.y);
	gl->glUniform2f(3, uv_offset.x, uv_offset.y);

	gl->glActiveTexture( GL_TEXTURE0 + 0 );
	gl->glBindTexture( GL_TEXTURE_2D, terrain.ground_corner_height );
	gl->glActiveTexture( GL_TEXTURE0 + 1 );
	gl->glBindTexture( GL_TEXTURE_2D, brush_texture );

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
	gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shapes.index_buffer);
	gl->glDrawElementsInstanced(GL_TRIANGLES, shapes.quad_indices.size() * 3, GL_UNSIGNED_INT, nullptr, cells * cells);

	gl->glDisableVertexAttribArray(0);
	gl->glEnable(GL_DEPTH_TEST);
}

void PathingBrush::apply(PathingMap& pathing) {
	int y = position.y * 4 + uv_offset.y;
	int x = position.x * 4 + uv_offset.x;
	int cells = this->size * 2 + 1;

	QRect area = QRect(x, y, cells, cells).intersected({ 0, 0, pathing.width, pathing.height });

	if (area.width() <= 0 || area.height() <= 0) {
		return;
	}

	int offset = area.y() * pathing.width + area.x();
	
	for (int i = 0; i < area.width(); i++) {
		for (int j = 0; j < area.height(); j++) {
			int index = offset + j * pathing.width + i;
			switch (operation) {
				case Operation::replace:
					pathing.pathing_cells[index] &= ~0b00001110;
					pathing.pathing_cells[index] |= brush_mask;
					break;
				case Operation::add:
					pathing.pathing_cells[index] |= brush_mask;
					break;
				case Operation::remove:
					pathing.pathing_cells[index] &= ~brush_mask;
					break;
			}
		}
	}

	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, pathing.width);
	gl->glBindTexture( GL_TEXTURE_2D, pathing.pathing_texture );
	gl->glTexSubImage2D( GL_TEXTURE_2D, 0, area.x( ), area.y( ), area.width( ), area.height( ), GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing.pathing_cells.data( ) + offset );
	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}