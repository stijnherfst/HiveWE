#include "stdafx.h"

CliffMesh::CliffMesh(const fs::path& path) {
	if (path.extension() == ".mdx" || path.extension() == ".MDX") {
		mdx::MDX model = mdx::MDX(BinaryReader(hierarchy.open_file(path)));

		auto set = model.chunk<mdx::GEOS>()->geosets.front();
		vertices = set.vertices.size();

		gl->glGenBuffers( 1, &vertexBuffer );
		gl->glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );
		gl->glBufferData( GL_ARRAY_BUFFER, set.vertices.size( ) * sizeof( glm::vec3 ), set.vertices.data( ), GL_STATIC_DRAW );

		gl->glGenBuffers( 1, &uvBuffer );
		gl->glBindBuffer( GL_ARRAY_BUFFER, uvBuffer );
		gl->glBufferData( GL_ARRAY_BUFFER, set.texture_coordinate_sets.front( ).coordinates.size( ) * sizeof( glm::vec2 ), set.texture_coordinate_sets.front( ).coordinates.data( ), GL_STATIC_DRAW );

		indices = set.faces.size();
		gl->glGenBuffers( 1, &indexBuffer );
		gl->glBindBuffer( GL_ARRAY_BUFFER, indexBuffer );
		gl->glBufferData( GL_ARRAY_BUFFER, set.faces.size( ) * sizeof( uint16_t ), set.faces.data( ), GL_STATIC_DRAW );

		gl->glGenBuffers( 1, &instanceBuffer );
		gl->glBindBuffer( GL_ARRAY_BUFFER, instanceBuffer );
	}
}

CliffMesh::~CliffMesh() {
	gl->glDeleteBuffers(1, &vertexBuffer);
	gl->glDeleteBuffers(1, &uvBuffer);
	gl->glDeleteBuffers(1, &indexBuffer);
	gl->glDeleteBuffers(1, &instanceBuffer);
}

void CliffMesh::render_queue(glm::vec4 position) {
	render_jobs.push_back(position);
}

void CliffMesh::render() {
	if (render_jobs.size() == 0) {
		return;
	}

	gl->glBindBuffer( GL_ARRAY_BUFFER, instanceBuffer );
	gl->glBufferData( GL_ARRAY_BUFFER, render_jobs.size( ) * sizeof( glm::vec4 ), render_jobs.data( ), GL_STATIC_DRAW );

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glEnableVertexAttribArray(1);
	gl->glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	gl->glEnableVertexAttribArray(2);
	gl->glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
	gl->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, 0);
	gl->glVertexAttribDivisor(2, 1);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	gl->glDrawElementsInstanced(GL_TRIANGLES, indices, GL_UNSIGNED_SHORT, nullptr, render_jobs.size());

	gl->glVertexAttribDivisor(2, 0); // ToDo use vao
	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);
	gl->glDisableVertexAttribArray(2);

	render_jobs.clear();
}