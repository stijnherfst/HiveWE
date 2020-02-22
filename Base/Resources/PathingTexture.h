#pragma once

#include "ResourceManager.h"

#include <vector>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>

class PathingTexture : public Resource {
public:
	int width;
	int height;
	int channels;
	std::vector<uint8_t> data;

	bool homogeneous;

	static constexpr const char* name = "PathingTexture";

	explicit PathingTexture() = default;
	explicit PathingTexture(const fs::path& path);
};