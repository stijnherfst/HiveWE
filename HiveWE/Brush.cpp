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

	gl->glCreateTextures(GL_TEXTURE_2D, 1, &brush_texture);
	gl->glBindTexture(GL_TEXTURE_2D, brush_texture);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, cells * 4, cells * 4, 0, GL_BGRA, GL_UNSIGNED_BYTE, brush.data());
	gl->glTextureParameteri(brush_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl->glTextureParameteri(brush_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	shader = resource_manager.load<Shader>({ "Data/Shaders/brush.vs", "Data/Shaders/brush.fs" });
}

void Brush::set_position(const glm::vec2 position) {
	this->position = position;
	uv_offset = (position - glm::vec2(this->position)) * 4.f;
}

void Brush::set_size(const int size) {
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
}

void Brush::render(Terrain& terrain) {
	gl->glDisable(GL_DEPTH_TEST);

	shader->use();

	int cells = std::ceil((this->size * 2 + 1 + 3) / 4.f);

	gl->glUniformMatrix4fv(1, 1, GL_FALSE, &camera.projection_view[0][0]);
	gl->glUniform2f(2, position.x, position.y);
	gl->glUniform2f(3, uv_offset.x, uv_offset.y);

	gl->glBindTextureUnit(0, terrain.ground_height_texture);
	gl->glBindTextureUnit(1, brush_texture);

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
	if (x < 0 || x >= pathing.width || y < 0 || y >= pathing.height) {
		return;
	}

	int cells = this->size * 2 + 1;
	int offset = y * pathing.width + x;

	int width = std::clamp(cells, 0, pathing.width - x);
	int height = std::clamp(cells, 0, pathing.height - y);

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
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
	gl->glTextureSubImage2D(pathing.pathing_texture, 0, x, y, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing.pathing_cells.data() + offset);
	gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}