#pragma once

class Texture : public Resource {
public:
	int width;
	int height;
	int channels;
	std::vector<uint8_t> data;
	glm::vec4 clr;
	static constexpr const char* name = "Texture";

	explicit Texture() = default;
	explicit Texture(const fs::path& path);
};