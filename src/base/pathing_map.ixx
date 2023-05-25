module;

#include <memory>
#include <print>

#include <glad/glad.h>
#include <QRect>

#include <glm/glm.hpp>

export module PathingMap;

import BinaryReader;
import BinaryWriter;
import PathingTexture;
import TerrainUndo;
import OpenGLUtilities;
import Hierarchy;

export class PathingMap {
	static constexpr int write_version = 0;

  public:
	int width;
	int height;

	enum Flags {
		unwalkable = 0b00000010,
		unflyable = 0b00000100,
		unbuildable = 0b00001000,
	};

	GLuint texture_static;
	GLuint texture_dynamic;
	std::vector<uint8_t> pathing_cells_static;
	std::vector<uint8_t> pathing_cells_dynamic;

	// For undo/redo
	std::vector<uint8_t> old_pathing_cells_static;

	bool load(size_t terrain_width, size_t terrain_height) {
		BinaryReader reader = hierarchy.map_file_read("war3map.wpm");
		const std::string magic_number = reader.read_string(4);
		if (magic_number != "MP3W") {
			std::print("Invalid war3map.wpm magic number, expected MP3W but got {}", magic_number);
			return false;
		}

		const int version = reader.read<uint32_t>();
		if (version != 0) {
			std::print("Unknown war3map.wpm version, expected 0 but got {}. Attempting to load, but may crash.\n", version);
		}

		width = reader.read<uint32_t>();
		height = reader.read<uint32_t>();

		if (width == 0 || height == 0) {
			resize(terrain_width * 4, terrain_height * 4);
			return true;
		}

		pathing_cells_static = reader.read_vector<uint8_t>(width * height);
		pathing_cells_dynamic.resize(width * height);

		glCreateTextures(GL_TEXTURE_2D, 1, &texture_static);
		glTextureStorage2D(texture_static, 1, GL_R8UI, width, height);
		glTextureSubImage2D(texture_static, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing_cells_static.data());
		glTextureParameteri(texture_static, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(texture_static, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(texture_static, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture_static, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glCreateTextures(GL_TEXTURE_2D, 1, &texture_dynamic);
		glTextureStorage2D(texture_dynamic, 1, GL_R8UI, width, height);
		const uint8_t clear_color = 0;
		glClearTexImage(texture_dynamic, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear_color);
		glTextureParameteri(texture_dynamic, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(texture_dynamic, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(texture_dynamic, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture_dynamic, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		return true;
	}

	void save() const {
		BinaryWriter writer;
		writer.write_string("MP3W");
		writer.write<uint32_t>(write_version);
		writer.write<uint32_t>(width);
		writer.write<uint32_t>(height);
		writer.write_vector<uint8_t>(pathing_cells_static);

		hierarchy.map_file_write("war3map.wpm", writer.buffer);
	}

	/// Clears an area with zeroes
	void dynamic_clear_area(const QRect& area) {
		const QRect t = QRect(area.left() * 4, area.top() * 4, area.width() * 4, area.height() * 4).intersected({ 0, 0, width, height });

		for (int j = t.top(); j < t.bottom(); j++) {
			for (int i = t.left(); i < t.right(); i++) {
				pathing_cells_dynamic[j * width + i] = 0;
			}
		}
	}

	/// Checks for every cell on the supplied pathing_texture where (pathing_texture & mask == true) whether (existing_pathing & mask == true) and if so returns false
	/// Expects position in whole grid tiles
	/// Rotation in multiples of 90
	bool is_area_free(glm::vec2 position, int rotation, const std::shared_ptr<PathingTexture>& pathing_texture, uint8_t mask) {
		const int div_w = (rotation % 180) ? pathing_texture->height : pathing_texture->width;
		const int div_h = (rotation % 180) ? pathing_texture->width : pathing_texture->height;
		for (int j = 0; j < pathing_texture->height; j++) {
			for (int i = 0; i < pathing_texture->width; i++) {
				int x = i;
				int y = j;

				switch (rotation) {
					case 90:
						x = pathing_texture->height - 1 - j;
						y = i;
						break;
					case 180:
						x = pathing_texture->width - 1 - i;
						y = pathing_texture->height - 1 - j;
						break;
					case 270:
						x = j;
						y = pathing_texture->width - 1 - i;
						break;
				}

				// Width and height for centering change if rotation is not divisible by 180
				const int xx = position.x * 4 + x - div_w / 2;
				const int yy = position.y * 4 + y - div_h / 2;

				if (xx < 0 || xx > width - 1 || yy < 0 || yy > height - 1) {
					continue;
				}

				const unsigned int index = ((pathing_texture->height - 1 - j) * pathing_texture->width + i) * pathing_texture->channels;

				uint8_t pathing_texture_mask = (pathing_texture->data[index] > 250) * Flags::unwalkable | (pathing_texture->data[index + 1] > 250) * Flags::unflyable | (pathing_texture->data[index + 2] > 250) * Flags::unbuildable;

				if (pathing_texture_mask & mask && pathing_cells_dynamic[yy * width + xx] && mask) {
					return false;
				}
			}
		}
		return true;
	}

	/// Blits a pathing texture to the specified location on the pathing map. Manually call update_dynamic() afterwards to upload the changes to the GPU
	/// Expects position in whole grid tiles and draws the texture centered around this position
	/// Rotation in multiples of 90
	/// Blits the texture upside down as OpenGL uses the bottom-left as 0,0
	void blit_pathing_texture(glm::vec2 position, int rotation, const std::shared_ptr<PathingTexture>& pathing_texture) {
		const int div_w = (rotation % 180) ? pathing_texture->height : pathing_texture->width;
		const int div_h = (rotation % 180) ? pathing_texture->width : pathing_texture->height;
		for (int j = 0; j < pathing_texture->height; j++) {
			for (int i = 0; i < pathing_texture->width; i++) {
				int x = i;
				int y = j;

				switch (rotation) {
					case 90:
						x = pathing_texture->height - 1 - j;
						y = i;
						break;
					case 180:
						x = pathing_texture->width - 1 - i;
						y = pathing_texture->height - 1 - j;
						break;
					case 270:
						x = j;
						y = pathing_texture->width - 1 - i;
						break;
				}

				// Width and height for centering change if rotation is not divisible by 180
				const int xx = position.x * 4 + x - div_w / 2;
				const int yy = position.y * 4 + y - div_h / 2;

				if (xx < 0 || xx > width - 1 || yy < 0 || yy > height - 1) {
					continue;
				}

				const unsigned int index = ((pathing_texture->height - 1 - j) * pathing_texture->width + i) * pathing_texture->channels;

				uint8_t bytes = (pathing_texture->data[index] > 250) * Flags::unwalkable | (pathing_texture->data[index + 1] > 250) * Flags::unflyable | (pathing_texture->data[index + 2] > 250) * Flags::unbuildable;

				pathing_cells_dynamic[yy * width + xx] |= bytes;
			}
		}
	}

	void upload_static_pathing() {
		glTextureSubImage2D(texture_static, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing_cells_static.data());
	}

	void upload_dynamic_pathing() {
		glTextureSubImage2D(texture_dynamic, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing_cells_dynamic.data());
	}

	void new_undo_group() {
		old_pathing_cells_static = pathing_cells_static;
	}

	// Undo/redo structures
	class PathingMapAction : public TerrainUndoAction {
	  public:
		QRect area;
		std::vector<uint8_t> old_pathing;
		std::vector<uint8_t> new_pathing;
		PathingMap& pathing_map;

		PathingMapAction(PathingMap& pathing_map)
			: pathing_map(pathing_map) {
		}

		void undo() override {
			for (int j = area.top(); j <= area.bottom(); j++) {
				for (int i = area.left(); i <= area.right(); i++) {
					pathing_map.pathing_cells_static[j * pathing_map.width + i] = old_pathing[(j - area.top()) * area.width() + i - area.left()];
				}
			}
			pathing_map.upload_static_pathing();
		}

		void redo() override {
			for (int j = area.top(); j <= area.bottom(); j++) {
				for (int i = area.left(); i <= area.right(); i++) {
					pathing_map.pathing_cells_static[j * pathing_map.width + i] = new_pathing[(j - area.top()) * area.width() + i - area.left()];
				}
			}
			pathing_map.upload_static_pathing();
		}
	};

	std::unique_ptr<PathingMapAction> add_undo(const QRect& area) {
		auto undo_action = std::make_unique<PathingMapAction>(*this);

		undo_action->area = area;

		// Copy old corners
		undo_action->old_pathing.reserve(area.width() * area.height());
		for (int j = area.top(); j <= area.bottom(); j++) {
			for (int i = area.left(); i <= area.right(); i++) {
				undo_action->old_pathing.push_back(old_pathing_cells_static[j * width + i]);
			}
		}

		// Copy new corners
		undo_action->new_pathing.reserve(area.width() * area.height());
		for (int j = area.top(); j <= area.bottom(); j++) {
			for (int i = area.left(); i <= area.right(); i++) {
				undo_action->new_pathing.push_back(pathing_cells_static[j * width + i]);
			}
		}

		//map->terrain_undo.add_undo_action(std::move(undo_action));
		return std::move(undo_action);
	}

	void resize(size_t new_width, size_t new_height) {
		width = new_width;
		height = new_height;

		pathing_cells_static.resize(width * height);
		pathing_cells_dynamic.resize(width * height);

		old_pathing_cells_static.resize(width * height);

		glDeleteTextures(1, &texture_static);
		glCreateTextures(GL_TEXTURE_2D, 1, &texture_static);
		glTextureStorage2D(texture_static, 1, GL_R8UI, width, height);
		glTextureSubImage2D(texture_static, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, pathing_cells_static.data());
		glTextureParameteri(texture_static, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(texture_static, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(texture_static, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture_static, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glDeleteTextures(1, &texture_dynamic);
		glCreateTextures(GL_TEXTURE_2D, 1, &texture_dynamic);
		glTextureStorage2D(texture_dynamic, 1, GL_R8UI, width, height);
		const uint8_t clear_color = 0;
		glClearTexImage(texture_dynamic, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear_color);
		glTextureParameteri(texture_dynamic, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(texture_dynamic, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(texture_dynamic, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(texture_dynamic, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
};