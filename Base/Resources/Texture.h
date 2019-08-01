#pragma once

#include "ResourceManager.h"

#include <vector>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>

class Texture : public Resource {
public:
	int width;
	int height;
	int channels;
	std::vector<uint8_t> data;
	glm::vec4 minimap_color;
	static constexpr const char* name = "Texture";

	explicit Texture() = default;
	explicit Texture(const fs::path& path);
};