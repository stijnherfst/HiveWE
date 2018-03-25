#pragma once

class Texture : public Resource {
public:
	int width;
	int height;
	int channels;
	uint8_t* data;

	static constexpr const char* name = "Texture";

	explicit Texture(const fs::path& path);

	virtual ~Texture() {
		delete[] data;
	}
};

