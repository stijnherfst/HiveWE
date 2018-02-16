#pragma once

class Texture : public Resource {
public:
	int width;
	int height;
	int channels;
	uint8_t* data;

	static constexpr const char* name = "Texture";

	Texture(const fs::path& path);

	virtual ~Texture() {
		delete[] data;
	}
};

class GPUTexture : public Resource {
public:
	GLuint id = 0;

	static constexpr const char* name = "GPUTexture";

	GPUTexture(const fs::path& path);

	virtual ~GPUTexture() {
		gl->glDeleteTextures(1, &id);
	}
};