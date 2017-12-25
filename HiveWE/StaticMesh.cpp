#include "stdafx.h"

StaticMesh::StaticMesh(const std::string& path) {
	if (fs::path(path).extension() == ".mdx" || fs::path(path).extension() == ".MDX") {
		mdx::MDX model = mdx::MDX(BinaryReader(hierarchy.open_file(path)));

		auto set = dynamic_cast<mdx::GEOS*>(model.chunks[mdx::ChunkTag::GEOS])->geosets.front();
		vertices = set.vertices.size();

		gl->glCreateBuffers(1, &vertexBuffer);
		gl->glNamedBufferData(vertexBuffer, set.vertices.size() * sizeof(float), set.vertices.data(), GL_STATIC_DRAW);

		gl->glCreateBuffers(1, &uvBuffer);
		gl->glNamedBufferData(uvBuffer, set.texture_coordinate_sets.front().coordinates.size() * sizeof(float), set.texture_coordinate_sets.front().coordinates.data(), GL_STATIC_DRAW);

		indices = set.faces.size();
		gl->glCreateBuffers(1, &indexBuffer);
		gl->glNamedBufferData(indexBuffer, set.faces.size() * sizeof(uint16_t), set.faces.data(), GL_STATIC_DRAW);

		auto texs = dynamic_cast<mdx::TEXS*>(model.chunks[mdx::ChunkTag::TEXS])->textures.front();

		if (texs.file_name != "") {
			auto tex = resource_manager.load<Texture>(texs.file_name);

			gl->glCreateTextures(GL_TEXTURE_2D, 1, &texture);
			gl->glTextureStorage2D(texture, 0, GL_RGBA, tex->width, tex->height);
			gl->glTextureSubImage2D(texture, 0, 0, 0, tex->width, tex->height, GL_RGBA, GL_UNSIGNED_BYTE, tex->data);
			gl->glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			gl->glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			gl->glTextureParameteri(texture, GL_TEXTURE_WRAP_S, (texs.flags & 1) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
			gl->glTextureParameteri(texture, GL_TEXTURE_WRAP_T, (texs.flags & 2) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
			gl->glGenerateMipmap(GL_TEXTURE_2D);
		}
	}
}

void StaticMesh::render() {
	gl->glBindTextureUnit(0, texture);

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