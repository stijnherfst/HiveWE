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
	this->position = glm::ivec2(position);
	uv_offset = glm::ivec2((position - this->position) * 4.f);
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

void PathingBrush::apply(Terrain& terrain) {
	//int x = std::clamp((int)position.x - size, 0, terrain.width);
	//int y = std::k((int)position.y - size, 0, terrain.height);
	//int width = std::clamp((int)position.x + size, 0, terrain.width);
	//int height = std::clamp((int)position.x + size, 0, terrain.width);

	//for (int i = x; i < )

	//uint8_t byte = reader.read<uint8_t>();
	//walkable = !(byte & 0b00000010);
	//flyable = !(byte & 0b00000100);
	//buildable = !(byte & 0b00001000);
	//blight = !(byte & 0b00100000);
	//water = !(byte & 0b01000000);
	//unknown = !(byte & 0b10000000);


	unsigned char dataa = 0b11111111;
	gl->glTextureSubImage2D(terrain.pathing_map_texture, 0, position.x * 4 + uv_offset.x, position.y * 4 + uv_offset.y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &dataa);

}