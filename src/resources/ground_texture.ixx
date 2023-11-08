module;

#include <soil2/SOIL2.h>
#include <filesystem>
#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>

#include <glad/glad.h>

export module GroundTexture;

namespace fs = std::filesystem;

import ResourceManager;
import OpenGLUtilities;
import BLP;
import Hierarchy;

export class GroundTexture : public Resource {
  public:
	GLuint id = 0;
	int tile_size;
	bool extended = false;
	glm::vec4 minimap_color;

	static constexpr const char* name = "GroundTexture";

	explicit GroundTexture(const fs::path& path) {
		fs::path new_path = path;

		if (hierarchy.hd) {
			new_path.replace_filename(path.stem().string() + "_diffuse.dds");
		}
		if (!hierarchy.file_exists(new_path)) {
			new_path = path;
			new_path.replace_extension(".blp");
			if (!hierarchy.file_exists(new_path)) {
				new_path.replace_extension(".dds");
			}
		}

		BinaryReader reader = hierarchy.open_file(new_path);

		int width;
		int height;
		int channels;
		uint8_t* data;

		if (new_path.extension() == ".blp" || new_path.extension() == ".BLP") {
			data = blp::load(reader, width, height, channels);
		} else {
			data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
		}

		tile_size = height * 0.25;
		extended = (width == height * 2);
		int lods = log2(tile_size) + 1;

		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id);
		glTextureStorage3D(id, lods, GL_RGBA8, tile_size, tile_size, extended ? 32 : 16);
		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				glTextureSubImage3D(id, 0, 0, 0, y * 4 + x, tile_size, tile_size, 1, GL_RGBA, GL_UNSIGNED_BYTE, data + (y * tile_size * width + x * tile_size) * 4);

				if (extended) {
					glTextureSubImage3D(id, 0, 0, 0, y * 4 + x + 16, tile_size, tile_size, 1, GL_RGBA, GL_UNSIGNED_BYTE, data + (y * tile_size * width + (x + 4) * tile_size) * 4);
				}
			}
		}
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glGenerateTextureMipmap(id);

		glGetTextureSubImage(id, lods - 1, 0, 0, 0, 1, 1, 1, GL_RGBA, GL_FLOAT, 16, &minimap_color);
		minimap_color *= 255.f;

		delete data;
	}

	virtual ~GroundTexture() {
		glDeleteTextures(1, &id);
	}
};