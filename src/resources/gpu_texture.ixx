export module GPUTexture;

import std;
import types;
import BinaryReader;
import ResourceManager;
import Hierarchy;
import BLP;
import Timer;
import <soil2/SOIL2.h>;
import <glad/glad.h>;

namespace fs = std::filesystem;

export class GPUTexture : public Resource {
  public:
	GLuint id = 0;
	GLuint64 bindless_handle = 0;
	bool handle_resident = false; // Singals the render context to set residency as it is per-context

	static constexpr const char* name = "GPUTexture";

	/// Must be called from the rendering context
	void make_resident() {
		if (!handle_resident && bindless_handle != 0) {
			glMakeTextureHandleResidentARB(bindless_handle);
			handle_resident = true;
		}
	}

	explicit GPUTexture(const fs::path& path, int flags = 0) {
		fs::path new_path = path;

		BinaryReader reader = [&] {
			ScopedTimer t(profile_casc_ns);
			new_path.replace_extension(".tga");
			return hierarchy.open_file(new_path)
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
		}();

		if (new_path.extension() == ".blp") {
			const auto image = blp::load(reader);

			if (image) {
				glCreateTextures(GL_TEXTURE_2D, 1, &id);
				glTextureStorage2D(id, std::log2(std::max(image->width, image->height)) + 1, GL_RGBA8, image->width, image->height);
				glTextureSubImage2D(id, 0, 0, 0, image->width, image->height, GL_RGBA, GL_UNSIGNED_BYTE, image->data.data());
				glGenerateTextureMipmap(id);
			} else {
				glCreateTextures(GL_TEXTURE_2D, 1, &id);
				std::println("Error loading BLP texture {}: {}", path.string(), image.error());
			}
		} else {
			id = SOIL_load_OGL_texture_from_memory(
				reader.buffer.data(),
				static_cast<int>(reader.buffer.size()),
				SOIL_LOAD_AUTO,
				SOIL_LOAD_AUTO,
				SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_SRGB_COLOR_SPACE | SOIL_FLAG_GL_MIPMAPS
			);
			if (id == 0) {
				glCreateTextures(GL_TEXTURE_2D, 1, &id);
				std::println("Error loading texture: {}", path.string());
			}
		}

		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_WRAP_S, (flags & 1) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, (flags & 2) ? GL_REPEAT : GL_CLAMP_TO_EDGE);

		// Make resident later on the rendering context
		bindless_handle = glGetTextureHandleARB(id);
	}

	virtual ~GPUTexture() {
		if (handle_resident) {
			glMakeTextureHandleNonResidentARB(bindless_handle);
		}
		glDeleteTextures(1, &id);
	}
};