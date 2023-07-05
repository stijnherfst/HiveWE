module;

#include <soil2/SOIL2.h>
#include <filesystem>
#include <glad/glad.h>
#include <iostream>

export module GPUTexture;

namespace fs = std::filesystem;

import BinaryReader;
import ResourceManager;
import Hierarchy;
import BLP;

export class GPUTexture : public Resource {
  public:
	GLuint id = 0;
	//GLuint64 bindless_handle = 0;

	static constexpr const char* name = "GPUTexture";

	explicit GPUTexture(const fs::path& path) {
		fs::path new_path = path;

		if (hierarchy.hd) {
			new_path.replace_filename(path.stem().string() + "_diffuse.dds");
		}
		if (!hierarchy.file_exists(new_path)) {
			new_path = path;
			new_path.replace_extension(".blp");
			if (!hierarchy.file_exists(new_path)) {
				new_path.replace_extension(".dds");
				if (!hierarchy.file_exists(new_path)) {
					std::cout << "Error loading texture " << new_path << "\n";
					new_path = "Textures/btntempw.dds";
				}
			}
		}

		BinaryReader reader = hierarchy.open_file(new_path);

		if (new_path.extension() == ".blp" || new_path.extension() == ".BLP") {
			int width;
			int height;
			int channels;
			uint8_t* data = blp::load(reader, width, height, channels);

			glCreateTextures(GL_TEXTURE_2D, 1, &id);
			glTextureStorage2D(id, log2(std::max(width, height)) + 1, GL_RGBA8, width, height);
			glTextureSubImage2D(id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateTextureMipmap(id);
			delete data;
		} else {
			id = SOIL_load_OGL_texture_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), SOIL_LOAD_AUTO, SOIL_LOAD_AUTO, SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_SRGB_COLOR_SPACE);
			if (id == 0) {
				glCreateTextures(GL_TEXTURE_2D, 1, &id);
				std::cout << "Error loading texture: " << path << "\n";
			}
		}

		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		//bindless_handle = glGetTextureHandleARB(id);
		//glMakeTextureHandleResidentARB(bindless_handle);
	}

	virtual ~GPUTexture() {
		glDeleteTextures(1, &id);
	}
};