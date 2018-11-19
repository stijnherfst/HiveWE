#pragma once

class GroundTexture : public Resource {
public:
	GLuint id = 0;
	int tile_size;
	bool extended = false;
	glm::vec4 minimap_color;

	static constexpr const char* name = "GroundTexture";

	explicit GroundTexture(const fs::path& path);

	virtual ~GroundTexture() {
		gl->glDeleteTextures(1, &id);
	}
};