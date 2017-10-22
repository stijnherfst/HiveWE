#pragma once

class Texture : public Resource {
public:
	int width;
	int height;
	int channels;
	uint8_t* data;

	Texture(const std::string& path) : Resource(path) {
		load(path);
	}

	virtual ~Texture() {
		unload();
	}
protected:
	void load(const std::string& path);

	void unload();
};