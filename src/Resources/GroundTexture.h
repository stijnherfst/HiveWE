#pragma once

import ResourceManager;

#include "OpenGLUtilities.h"
#include <QOpenGLFunctions_4_5_Core>

#define GLM_FORCE_CXX17
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>

#include <filesystem>
namespace fs = std::filesystem;

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