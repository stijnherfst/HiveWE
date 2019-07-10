#pragma once

#include "utilities.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "ResourceManager.h"

class GPUTexture : public Resource {
public:
	GLuint id = 0;

	static constexpr const char* name = "GPUTexture";

	explicit GPUTexture(const fs::path& path);

	virtual ~GPUTexture() {
		gl->glDeleteTextures(1, &id);
	}
};