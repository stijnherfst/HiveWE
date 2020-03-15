#pragma once

#include "ResourceManager.h"

#include <vector>

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