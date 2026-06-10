export module GroundTexture;

import std;
import BinaryReader;
import ResourceManager;
import OpenGLUtilities;
import BLP;
import Hierarchy;
import <soil2/SOIL2.h>;
import <glm/glm.hpp>;
import <glad/glad.h>;

namespace fs = std::filesystem;

export class GroundTexture : public Resource {
  public:
	GLuint id = 0;
	GLuint64 bindless_handle = 0;
	int tile_size;
	bool extended = false;
	glm::vec4 minimap_color;

	static constexpr const char* name = "GroundTexture";

	explicit GroundTexture(const fs::path& path) {
		fs::path new_path = path;

		if (hierarchy.hd) {
			new_path.replace_filename(path.stem().string() + "_diffuse");
		}

		new_path.replace_extension(".tga");
		BinaryReader reader = hierarchy.open_file(new_path)
			.or_else([&](const std::string&) {
				new_path.replace_extension(".blp");
				return hierarchy.open_file(new_path);
			})
			.or_else([&](const std::string&) {
				new_path.replace_extension(".dds");
				return hierarchy.open_file(new_path);
			})
			.or_else([&](const std::string&) {
				std::println("Error loading texture {}", new_path.string());
				new_path = "Textures/btntempw.dds";
				return hierarchy.open_file(new_path);
			})
			.value();

		int width = 0;
		int height = 0;
		int channels = 0;

		std::vector<uint8_t> blp_data;
		uint8_t* soil_data = nullptr;
		// To avoid intermediate memcpy
		const uint8_t* pixels = nullptr;

		if (new_path.extension() == ".blp") {
			auto image = blp::load(reader);
			if (!image) {
				throw std::runtime_error(std::format("Failed to load ground texture {}: {}", new_path.string(), image.error()));
			}
			width = image->width;
			height = image->height;
			channels = image->channels;
			blp_data = std::move(image->data);
			pixels = blp_data.data();
		} else {
			soil_data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
			if (soil_data == nullptr) {
				throw std::runtime_error(std::format("Failed to decode ground texture {}", new_path.string()));
			}
			pixels = soil_data;
		}

		tile_size = std::max(height * 0.25f, 1.f);
		extended = (width == height * 2);
		const int lods = log2(tile_size) + 1;

		const int format = channels == 3 ? GL_RGB : GL_RGBA;
		const int bit_format = channels == 3 ? GL_RGB8 : GL_RGBA8;

		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id);
		glTextureStorage3D(id, lods, bit_format, tile_size, tile_size, extended ? 32 : 16);
		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				glTextureSubImage3D(id, 0, 0, 0, y * 4 + x, tile_size, tile_size, 1, format, GL_UNSIGNED_BYTE, pixels + (y * tile_size * width + x * tile_size) * channels);

				if (extended) {
					glTextureSubImage3D(id, 0, 0, 0, y * 4 + x + 16, tile_size, tile_size, 1, format, GL_UNSIGNED_BYTE, pixels + (y * tile_size * width + (x + 4) * tile_size) * channels);
				}
			}
		}
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

		free(soil_data);

		glGenerateTextureMipmap(id);

		glGetTextureSubImage(id, lods - 1, 0, 0, 0, 1, 1, 1, format, GL_FLOAT, 16, &minimap_color);
		minimap_color *= 255.f;

		bindless_handle = glGetTextureHandleARB(id);
		glMakeTextureHandleResidentARB(bindless_handle);
	}

	virtual ~GroundTexture() {
		glMakeTextureHandleNonResidentARB(bindless_handle);
		glDeleteTextures(1, &id);
	}
};