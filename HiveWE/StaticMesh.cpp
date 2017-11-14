#include "stdafx.h"

StaticMesh::StaticMesh(const std::string& path) {
	auto t = fs::path(path).extension();
	if (fs::path(path).extension() == ".mdx" || fs::path(path).extension() == ".MDX") {
		mdx::MDX model = mdx::MDX(path);

		auto set = dynamic_cast<mdx::GEOS*>(model.chunks[mdx::ChunkTag::GEOS])->geosets.front();
		vertices = set.vertices.size();

		gl->glGenBuffers(1, &vertexBuffer);
		gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		gl->glBufferData(GL_ARRAY_BUFFER, set.vertices.size() * sizeof(float), set.vertices.data(), GL_STATIC_DRAW);

		gl->glGenBuffers(1, &uvBuffer);
		gl->glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
		gl->glBufferData(GL_ARRAY_BUFFER, set.texture_coordinate_sets.front().coordinates.size() * sizeof(float), set.texture_coordinate_sets.front().coordinates.data(), GL_STATIC_DRAW);

		//gl->glGenBuffers(1, &normalBuffer);
		//gl->glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
		//gl->glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
		indices = set.faces.size();
		gl->glGenBuffers(1, &indexBuffer);
		gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, set.faces.size() * sizeof(uint16_t), set.faces.data(), GL_STATIC_DRAW);

		auto texs = dynamic_cast<mdx::TEXS*>(model.chunks[mdx::ChunkTag::TEXS])->textures.front();

		if (texs.file_name != "") {
			auto tex = resource_manager.load<Texture>(texs.file_name);
			gl->glGenTextures(1, &texture);
			gl->glBindTexture(GL_TEXTURE_2D, texture);
			gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex->data);
			gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			gl->glGenerateMipmap(GL_TEXTURE_2D);
		}
	} else {

	}
}

void StaticMesh::render() {
	gl->glActiveTexture(GL_TEXTURE0);
	gl->glBindTexture(GL_TEXTURE_2D, texture);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glEnableVertexAttribArray(1);
	gl->glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	gl->glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_SHORT, NULL);

	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);
}