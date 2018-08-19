#include "stdafx.h"

void DoodadBrush::apply() {
	map.doodads.add_doodad(id, glm::vec2(position));
}

void DoodadBrush::render() const {
	glm::mat4 mat(1.f);

	mat = glm::translate(mat, glm::vec3(position.x, position.y, 0));
	mat = glm::scale(mat, glm::vec3(1.f / 128.f));

	if (mesh) {
		mesh->render_queue(mat);
	}
}