#pragma once

#include "Utilities.h"

#include <filesystem>
namespace fs = std::filesystem;

import ResourceManager;

class GPUTexture : public Resource {
public:
	GLuint id = 0;

	static constexpr const char* name = "GPUTexture";

	explicit GPUTexture(const fs::path& path);

	virtual ~GPUTexture() {
		gl->glDeleteTextures(1, &id);
	}
};