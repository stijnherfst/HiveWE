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


	gl->glCreateTextures(GL_TEXTURE_2D, 1, &brush_texture);
	gl->glTextureParameteri(brush_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl->glTextureParameteri(brush_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	set_size(size);

	shader = resource_manager.load<Shader>({ "Data/Shaders/brush.vs", "Data/Shaders/brush.fs" });
}

void Brush::set_position(const glm::vec2& position) {
	const glm::vec2 center_position = (position + brush_offset) - size * granularity * 0.25f;
	this->position = glm::floor(center_position) - glm::ceil(brush_offset);
	if (!uv_offset_locked) {
		uv_offset = glm::abs((center_position - glm::vec2(this->position)) * 4.f);
	}
}

void Brush::set_size(const int size) {
	const int change = size - this->size;

	this->size = std::clamp(size, 0, 240);
	const int cells = std::ceil(((this->size * 2 + 1) * granularity + 3) / 4.f);
	brush.clear();
	brush.resize(cells * 4 * cells * 4, { 0, 0, 0, 0 });

	for (int i = 0; i < this->size * 2 + 1; i++) {
		for (int j = 0; j < this->size * 2 + 1; j++) {
			for (int k = 0; k < granularity; k++) {
				for (int l = 0; l < granularity; l++) {
					brush[(j * granularity + l) * cells * 4 + i * granularity + k] = { 0, 255, 0, 128 };
				}
			}
		}
	}

	gl->glBindTexture(GL_TEXTURE_2D, brush_texture);
	gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, cells * 4, cells * 4, 0, GL_BGRA, GL_UNSIGNED_BYTE, brush.data());

	set_position(glm::vec2(position) + glm::vec2(uv_offset + size * granularity - change * granularity) * 0.25f);
}

void Brush::increase_size(const int size) {
	set_size(this->size + size);
}
void Brush::decrease_size(const int size) {
	set_size(this->size - size);
}

void Brush::render(Terrain& terrain) const {
	const glm::ivec2 center_position = position + (size + uv_offset) / 4;
	if (center_position.x < 0 || center_position.y < 0 || center_position.x > terrain.width || center_position.y > terrain.height) {
		return;
	}

	gl->glDisable(GL_DEPTH_TEST);

	shader->use();

	// +3 for uv_offset so it can move sub terrain cell
	const int cells = std::ceil(((this->size * 2 + 1) * granularity + 3) / 4.f);

	gl->glUniformMatrix4fv(1, 1, GL_FALSE, &camera.projection_view[0][0]);
	gl->glUniform2f(2, position.x, position.y);
	gl->glUniform2f(3, uv_offset.x, uv_offset.y);

	gl->glBindTextureUnit(0, terrain.ground_corner_height);
	gl->glBindTextureUnit(1, brush_texture);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
	gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shapes.index_buffer);
	gl->glDrawElementsInstanced(GL_TRIANGLES, shapes.quad_indices.size() * 3, GL_UNSIGNED_INT, nullptr, cells * cells);

	gl->glDisableVertexAttribArray(0);
	gl->glEnable(GL_DEPTH_TEST);
}