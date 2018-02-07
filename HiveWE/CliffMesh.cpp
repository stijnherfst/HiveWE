#include "stdafx.h"

CliffMesh::CliffMesh(const std::string& path) {
	if (fs::path(path).extension() == ".mdx" || fs::path(path).extension() == ".MDX") {
		mdx::MDX model = mdx::MDX(BinaryReader(hierarchy.open_file(path)));

		auto set = model.chunk<mdx::GEOS>()->geosets.front();
		vertices = set.vertices.size();

		gl->glCreateBuffers(1, &vertexBuffer);
		gl->glNamedBufferData(vertexBuffer, set.vertices.size() * sizeof(glm::vec3), set.vertices.data(), GL_STATIC_DRAW);

		gl->glCreateBuffers(1, &uvBuffer);
		gl->glNamedBufferData(uvBuffer, set.texture_coordinate_sets.front().coordinates.size() * sizeof(glm::vec2), set.texture_coordinate_sets.front().coordinates.data(), GL_STATIC_DRAW);

		indices = set.faces.size();
		gl->glCreateBuffers(1, &indexBuffer);
		gl->glNamedBufferData(indexBuffer, set.faces.size() * sizeof(uint16_t), set.faces.data(), GL_STATIC_DRAW);
	}
}

CliffMesh::~CliffMesh() {
	gl->glDeleteBuffers(1, &vertexBuffer);
	gl->glDeleteBuffers(1, &uvBuffer);
	gl->glDeleteBuffers(1, &indexBuffer);
}

void CliffMesh::render() {
	gl->glBindTextureUnit(0, texture->id);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glEnableVertexAttribArray(1);
	gl->glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	gl->glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_SHORT, nullptr);

	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);
}