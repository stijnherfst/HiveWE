module;

#include <QObject>
#include <QRect>

#include <brush.h>

export module Terrain;

import std;
import GroundTexture;
import Texture;
import BinaryReader;
import BinaryWriter;
import CliffMesh;
import Shader;
import SLK;
import Physics;
import PathingMap;
import Hierarchy;
import ResourceManager;
import Globals;
import Camera;
import UnorderedMap;
import "glad/glad.h";
import "glm/glm.hpp";
import "glm/gtc/matrix_transform.hpp";
import "glm/gtc/quaternion.hpp";
import "bullet/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h";
import "btBulletDynamicsCommon.h";

using namespace std::literals::string_literals;

export struct Corner {
	bool map_edge = false;
	uint8_t ground_texture = 0;
	float height = 0.f;
	float water_height = 0.f;
	bool ramp = false;
	bool blight = false;
	bool water = false;
	bool boundary = false;
	bool cliff = false;
	bool romp = false;
	bool special_doodad = false;
	uint8_t ground_variation = 0;
	uint8_t cliff_variation = 0;
	uint8_t cliff_texture = 15;
	uint8_t layer_height = 2;
};

// total sum 570
constexpr std::tuple<int, int> variation_chances[18] = {
	{0, 85},
	{16, 85},
	{0, 85},
	{1, 10},
	{2, 4},
	{3, 1},
	{4, 85},
	{5, 10},
	{6, 4},
	{7, 1},
	{8, 85},
	{9, 10},
	{10, 4},
	{11, 1},
	{12, 85},
	{13, 10},
	{14, 4},
	{15, 1}
};

export int random_ground_variation() {
	thread_local std::mt19937 rng(std::random_device {}());
	std::uniform_int_distribution<> dist(0, 570);
	int nr = dist(rng) - 1;
	for (auto&& [variation, chance] : variation_chances) {
		if (nr < chance) {
			return variation;
		}
		nr -= chance;
	}
	return 0;
}

export struct TilePathingg {
	bool unwalkable = false;
	bool unflyable = false;
	bool unbuildable = false;

	uint8_t mask() const {
		uint8_t mask = 0;
		mask |= unwalkable ? 0b00000010 : 0;
		mask |= unflyable ? 0b00000100 : 0;
		mask |= unbuildable ? 0b00001000 : 0;
		return mask;
	}
};

export class Terrain: public QObject {
	Q_OBJECT

	static constexpr int write_version = 12;

	// Derived GPU arrays
	std::vector<float> gpu_final_ground_heights;
	std::vector<glm::uvec4> gpu_ground_texture_list;
	std::vector<GLuint64> gpu_ground_texture_handles;
	/// One per tile, not per corner. So (width - 1) * (height - 1)
	std::vector<std::uint32_t> gpu_ground_exists_data;
	std::vector<uint32_t> gpu_water_exists_data;

	btHeightfieldTerrainShape* collision_shape;
	btRigidBody* collision_body;

	// Ground
	std::shared_ptr<Shader> ground_shader;
	std::vector<std::shared_ptr<GroundTexture>> ground_textures;

	// GPU buffers
	GLuint ground_height_buffer;
	GLuint cliff_level_buffer;
	GLuint water_height_buffer;
	GLuint ground_texture_handle_buffer;
	GLuint ground_texture_data_buffer;
	GLuint ground_exists_buffer;
	GLuint water_exists_buffer;

  public:
	static constexpr float min_ground_height = -16.f;
	static constexpr float max_ground_height = 15.98f; // ToDo why 15.98?

	static constexpr int min_layer_height = 0;
	static constexpr int max_layer_height = 15;

	char tileset;
	std::vector<std::string> tileset_ids;
	std::vector<std::string> cliffset_ids;

	/// In corners, not tiles
	int width;
	/// In corners, not tiles
	int height;
	/// In Warcraft units (*128)
	glm::vec2 offset;

	hive::unordered_map<std::string, int> ground_texture_to_id;
	hive::unordered_map<std::string, TilePathingg> pathing_options;

	// SoA corner data — indexed as ci(x, y) = y * width + x
	std::vector<float> corner_height;
	std::vector<float> corner_water_height;
	std::vector<uint8_t> corner_ground_texture;
	std::vector<uint8_t> corner_ground_variation;
	std::vector<uint8_t> corner_cliff_variation;
	std::vector<uint8_t> corner_cliff_texture;
	std::vector<uint8_t> corner_layer_height;
	std::vector<uint8_t> corner_map_edge;
	std::vector<uint8_t> corner_ramp;
	std::vector<uint8_t> corner_blight;
	std::vector<uint8_t> corner_water;
	std::vector<uint8_t> corner_boundary;
	std::vector<uint8_t> corner_cliff;
	/// Not the greatest name, but this indicates whether this tile contains a cliff transition.
	/// As cliff transitions stick out 1 tile from the cliff, the cliff flag is not set for half the `romps`
	std::vector<uint8_t> corner_romp;
	std::vector<uint8_t> corner_special_doodad;

	size_t ci(const size_t x, const size_t y) const {
		return y * width + x;
	}

	float corner_final_ground_height(const int x, const int y) const {
		const size_t idx = ci(x, y);
		return corner_height[idx] + corner_layer_height[idx] - 2.0f;
	}

	float corner_final_water_height(const int x, const int y) const {
		return corner_water_height[ci(x, y)] + water_offset;
	}

	Corner get_corner(const int x, const int y) const {
		const size_t idx = ci(x, y);
		Corner c;
		c.height = corner_height[idx];
		c.water_height = corner_water_height[idx];
		c.ground_texture = corner_ground_texture[idx];
		c.ground_variation = corner_ground_variation[idx];
		c.cliff_variation = corner_cliff_variation[idx];
		c.cliff_texture = corner_cliff_texture[idx];
		c.layer_height = corner_layer_height[idx];
		c.map_edge = corner_map_edge[idx];
		c.ramp = corner_ramp[idx];
		c.blight = corner_blight[idx];
		c.water = corner_water[idx];
		c.boundary = corner_boundary[idx];
		c.cliff = corner_cliff[idx];
		c.romp = corner_romp[idx];
		c.special_doodad = corner_special_doodad[idx];
		return c;
	}

	void set_corner(const int x, const int y, const Corner& c) {
		const size_t idx = ci(x, y);
		corner_height[idx] = c.height;
		corner_water_height[idx] = c.water_height;
		corner_ground_texture[idx] = c.ground_texture;
		corner_ground_variation[idx] = c.ground_variation;
		corner_cliff_variation[idx] = c.cliff_variation;
		corner_cliff_texture[idx] = c.cliff_texture;
		corner_layer_height[idx] = c.layer_height;
		corner_map_edge[idx] = c.map_edge;
		corner_ramp[idx] = c.ramp;
		corner_blight[idx] = c.blight;
		corner_water[idx] = c.water;
		corner_boundary[idx] = c.boundary;
		corner_cliff[idx] = c.cliff;
		corner_romp[idx] = c.romp;
		corner_special_doodad[idx] = c.special_doodad;
	}

	void resize_corner_arrays(const size_t total) {
		corner_height.assign(total, 0.f);
		corner_water_height.assign(total, 0.f);
		corner_ground_texture.assign(total, 0);
		corner_ground_variation.assign(total, 0);
		corner_cliff_variation.assign(total, 0);
		corner_cliff_texture.assign(total, 15);
		corner_layer_height.assign(total, 2);
		corner_map_edge.assign(total, 0);
		corner_ramp.assign(total, 0);
		corner_blight.assign(total, 0);
		corner_water.assign(total, 0);
		corner_boundary.assign(total, 0);
		corner_cliff.assign(total, 0);
		corner_romp.assign(total, 0);
		corner_special_doodad.assign(total, 0);
	}

	int variation_size = 64;
	int blight_texture;

	slk::SLK terrain_slk;
	slk::SLK cliff_slk;

	// Cliffs
	std::vector<glm::ivec3> cliffs;
	hive::unordered_map<std::string, uint8_t> path_to_cliff;
	hive::unordered_map<std::string, uint8_t> cliff_variations;
	std::vector<uint8_t> cliff_to_ground_texture;

	std::shared_ptr<Shader> cliff_shader;
	std::vector<std::shared_ptr<CliffMesh>> cliff_meshes;
	std::vector<std::shared_ptr<Texture>> cliff_textures;

	GLuint cliff_texture_array;

	int cliff_texture_size = 256;

	// Water
	static constexpr float min_depth = 10.f / 128.f;
	static constexpr float deep_level = 64.f / 128.f;
	static constexpr float max_depth = 72.f / 128.f;

	glm::vec4 shallow_color_min;
	glm::vec4 shallow_color_max;
	glm::vec4 deep_color_min;
	glm::vec4 deep_color_max;

	float water_offset;
	int water_textures_nr;
	int animation_rate;

	std::shared_ptr<Shader> water_shader;

	float current_texture = 1.f;
	GLuint water_texture_array;

	~Terrain() override {
		glDeleteTextures(1, &cliff_texture_array);
		glDeleteTextures(1, &water_texture_array);

		glDeleteBuffers(1, &ground_height_buffer);
		glDeleteBuffers(1, &cliff_level_buffer);
		glDeleteBuffers(1, &water_height_buffer);
		glDeleteBuffers(1, &ground_texture_data_buffer);
		glDeleteBuffers(1, &ground_exists_buffer);
		glDeleteBuffers(1, &water_exists_buffer);

		//map->physics.dynamicsWorld->removeRigidBody(collision_body);
		//delete collision_body;
		//delete collision_shape;
	}

	bool load(const Physics& physics) {
		BinaryReader reader = hierarchy.map_file_read("war3map.w3e").value();

		const std::string magic_number = reader.read_string(4);
		if (magic_number != "W3E!") {
			std::cout << "Invalid war3map.w3e file: Magic number is not W3E!" << std::endl;
			return false;
		}

		const uint32_t version = reader.read<uint32_t>();

		tileset = reader.read<char>();
		reader.advance(4); // Custom tileset

		const uint32_t tileset_textures = reader.read<uint32_t>();
		for (size_t i = 0; i < tileset_textures; i++) {
			tileset_ids.push_back(reader.read_string(4));
		}

		const int cliffset_textures = reader.read<uint32_t>();
		for (int i = 0; i < cliffset_textures; i++) {
			cliffset_ids.push_back(reader.read_string(4));
		}

		width = reader.read<uint32_t>();
		height = reader.read<uint32_t>();

		offset = reader.read<glm::vec2>();

		// Parse all tilepoints
		resize_corner_arrays(width * height);
		for (size_t i = 0; i < width * height; i++) {
			corner_height[i] = (reader.read<uint16_t>() - 8192.f) / 512.f;

			const uint16_t water_and_edge = reader.read<uint16_t>();
			corner_water_height[i] = ((water_and_edge & 0x3FFF) - 8192.f) / 512.f;
			corner_map_edge[i] = (water_and_edge & 0x4000) != 0;

			if (version >= 12) {
				const uint16_t texture_and_flags = reader.read<uint16_t>();
				corner_ground_texture[i] = texture_and_flags & 0b00'0000'0011'1111;

				corner_ramp[i] = (texture_and_flags & 0b00'0100'0000) != 0;
				corner_blight[i] = (texture_and_flags & 0b00'1000'0000) != 0;
				corner_water[i] = (texture_and_flags & 0b01'0000'0000) != 0;
				corner_boundary[i] = (texture_and_flags & 0b10'0000'0000) != 0;
			} else {
				const uint8_t texture_and_flags = reader.read<uint8_t>();
				corner_ground_texture[i] = texture_and_flags & 0b0000'1111;

				corner_ramp[i] = (texture_and_flags & 0b0001'0000) != 0;
				corner_blight[i] = (texture_and_flags & 0b0010'0000) != 0;
				corner_water[i] = (texture_and_flags & 0b0100'0000) != 0;
				corner_boundary[i] = (texture_and_flags & 0b1000'0000) != 0;
			}

			const uint8_t variation = reader.read<uint8_t>();
			corner_ground_variation[i] = variation & 0b0001'1111;
			corner_cliff_variation[i] = (variation & 0b1110'0000) >> 5;

			const uint8_t misc = reader.read<uint8_t>();
			corner_cliff_texture[i] = (misc & 0b1111'0000) >> 4;
			corner_layer_height[i] = misc & 0b0000'1111;
		}

		create(physics);

		return true;
	}

	void create(const Physics& physics) {
		// Determine if cliff
		compute_cliff_flags();

		hierarchy.tileset = tileset;

		terrain_slk.load("TerrainArt/Terrain.slk");
		cliff_slk.load("TerrainArt/CliffTypes.slk");
		const slk::SLK water_slk("TerrainArt/Water.slk");

		// Water Textures and Colours

		water_offset = water_slk.data<float>("height", tileset + "Sha"s);
		water_textures_nr = water_slk.data<int>("numtex", tileset + "Sha"s);
		animation_rate = water_slk.data<int>("texrate", tileset + "Sha"s);

		int red = water_slk.data<int>("smin_r", tileset + "Sha"s);
		int green = water_slk.data<int>("smin_g", tileset + "Sha"s);
		int blue = water_slk.data<int>("smin_b", tileset + "Sha"s);
		int alpha = water_slk.data<int>("smin_a", tileset + "Sha"s);

		shallow_color_min = {red, green, blue, alpha};
		shallow_color_min /= 255.f;

		red = water_slk.data<int>("smax_r", tileset + "Sha"s);
		green = water_slk.data<int>("smax_g", tileset + "Sha"s);
		blue = water_slk.data<int>("smax_b", tileset + "Sha"s);
		alpha = water_slk.data<int>("smax_a", tileset + "Sha"s);

		shallow_color_max = {red, green, blue, alpha};
		shallow_color_max /= 255.f;

		red = water_slk.data<int>("dmin_r", tileset + "Sha"s);
		green = water_slk.data<int>("dmin_g", tileset + "Sha"s);
		blue = water_slk.data<int>("dmin_b", tileset + "Sha"s);
		alpha = water_slk.data<int>("dmin_a", tileset + "Sha"s);

		deep_color_min = {red, green, blue, alpha};
		deep_color_min /= 255.f;

		red = water_slk.data<int>("dmax_r", tileset + "Sha"s);
		green = water_slk.data<int>("dmax_g", tileset + "Sha"s);
		blue = water_slk.data<int>("dmax_b", tileset + "Sha"s);
		alpha = water_slk.data<int>("dmax_a", tileset + "Sha"s);

		deep_color_max = {red, green, blue, alpha};
		deep_color_max /= 255.f;

		// Cliff Meshes
		slk::SLK cliffs_variation_slk("data/warcraft/Cliffs.slk", true);
		for (size_t i = 0; i < cliffs_variation_slk.rows(); i++) {
			for (int j = 0; j < cliffs_variation_slk.data<int>("variations", i) + 1; j++) {
				std::string file_name =
					"Doodads/Terrain/Cliffs/Cliffs" + cliffs_variation_slk.index_to_row.at(i) + std::to_string(j) + ".mdx";
				cliff_meshes.push_back(resource_manager.load<CliffMesh>(file_name).value());
				path_to_cliff.emplace(
					cliffs_variation_slk.index_to_row.at(i) + std::to_string(j),
					static_cast<int>(cliff_meshes.size()) - 1
				);
			}
			cliff_variations.emplace(cliffs_variation_slk.index_to_row.at(i), cliffs_variation_slk.data<int>("variations", i));
		}

		// Ground textures
		for (const auto& tile_id : tileset_ids) {
			ground_textures.push_back(
				resource_manager.load<GroundTexture>(terrain_slk.data("dir", tile_id) + "/" + terrain_slk.data("file", tile_id)).value()
			);
			ground_texture_to_id.emplace(tile_id, static_cast<int>(ground_textures.size() - 1));
			gpu_ground_texture_handles.push_back(ground_textures.back()->bindless_handle);
		}
		blight_texture = static_cast<int>(ground_textures.size());
		ground_texture_to_id.emplace("blight", blight_texture);
		ground_textures.push_back(
			resource_manager.load<GroundTexture>(world_edit_data.data("TileSets", std::string(1, tileset), 1)).value()
		);
		gpu_ground_texture_handles.push_back(ground_textures.back()->bindless_handle);

		// Cliff Textures
		for (const auto& cliff_id : cliffset_ids) {
			cliff_textures.push_back(
				resource_manager.load<Texture>(cliff_slk.data("texdir", cliff_id) + "/" + cliff_slk.data("texfile", cliff_id)).value()
			);
			cliff_texture_size = std::max(cliff_texture_size, cliff_textures.back()->width);
			cliff_to_ground_texture.push_back(ground_texture_to_id[cliff_slk.data<std::string_view>("groundtile", cliff_id)]);
		}

		// Prepare and create GPU buffers
		setup_GPU_buffers();

		// Ground texture handle buffer (only needed during initial creation)
		glCreateBuffers(1, &ground_texture_handle_buffer);
		glNamedBufferStorage(
			ground_texture_handle_buffer,
			gpu_ground_texture_handles.size() * sizeof(GLuint64),
			gpu_ground_texture_handles.data(),
			GL_DYNAMIC_STORAGE_BIT
		);

		// Cliff
		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &cliff_texture_array);
		glTextureStorage3D(
			cliff_texture_array,
			log2(cliff_texture_size) + 1,
			GL_RGBA8,
			cliff_texture_size,
			cliff_texture_size,
			cliff_textures.size()
		);
		glTextureParameteri(cliff_texture_array, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		int sub = 0;
		for (const auto& i : cliff_textures) {
			glTextureSubImage3D(
				cliff_texture_array,
				0,
				0,
				0,
				sub,
				i->width,
				i->height,
				1,
				i->channels == 4 ? GL_RGBA : GL_RGB,
				GL_UNSIGNED_BYTE,
				i->data.data()
			);
			sub += 1;
		}
		glGenerateTextureMipmap(cliff_texture_array);

		// Water textures
		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &water_texture_array);
		glTextureStorage3D(water_texture_array, std::log(128) + 1, GL_RGBA8, 128, 128, water_textures_nr);
		glTextureParameteri(water_texture_array, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(water_texture_array, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		const std::string_view file_name = water_slk.data<std::string_view>("texfile", tileset + "Sha"s);
		for (int i = 0; i < water_textures_nr; i++) {
			// Hack to force loading of SD water textures till I implement a water shader
			const auto hd = hierarchy.hd;
			hierarchy.hd = false;
			const auto texture = resource_manager.load<Texture>(std::format("{}{:02}", file_name, i)).value();
			hierarchy.hd = hd;

			if (texture->width != 128 || texture->height != 128) {
				std::cout << "Odd water texture size detected of " << texture->width << " wide and " << texture->height << " high\n";
			}
			glTextureSubImage3D(
				water_texture_array,
				0,
				0,
				0,
				i,
				texture->width,
				texture->height,
				1,
				texture->channels == 4 ? GL_RGBA : GL_RGB,
				GL_UNSIGNED_BYTE,
				texture->data.data()
			);
		}
		glGenerateTextureMipmap(water_texture_array);

		ground_shader = resource_manager.load<Shader>({"data/shaders/terrain.vert", "data/shaders/terrain.frag"}).value();
		cliff_shader = resource_manager.load<Shader>({"data/shaders/cliff.vert", "data/shaders/cliff.frag"}).value();
		water_shader = resource_manager.load<Shader>({"data/shaders/water.vert", "data/shaders/water.frag"}).value();

		setup_collision_shape(physics);

		update_ground_heights({0, 0, width - 1, height - 1});
		update_cliff_meshes({0, 0, width - 1, height - 1});
		update_ground_textures({0, 0, width, height});
		update_water({0, 0, width - 1, height - 1});

		emit minimap_changed(minimap_image());
	}

	void save() const {
		BinaryWriter writer;
		writer.write_string("W3E!");
		writer.write(write_version);
		writer.write(tileset);
		writer.write(1);
		writer.write<uint32_t>(tileset_ids.size());
		writer.write_vector(tileset_ids);
		writer.write<uint32_t>(cliffset_ids.size());
		writer.write_vector(cliffset_ids);
		writer.write(width);
		writer.write(height);
		writer.write(offset);

		for (size_t i = 0; i < width * height; i++) {
			writer.write<uint16_t>(corner_height[i] * 512.f + 8192.f);

			uint16_t water_and_edge = corner_water_height[i] * 512.f + 8192.f;
			water_and_edge += corner_map_edge[i] << 14;
			writer.write(water_and_edge);

			uint16_t texture_and_flags = corner_ground_texture[i];
			texture_and_flags |= corner_ramp[i] << 6;
			texture_and_flags |= corner_blight[i] << 7;
			texture_and_flags |= corner_water[i] << 8;
			texture_and_flags |= corner_boundary[i] << 9;
			writer.write(texture_and_flags);

			uint8_t variation = corner_ground_variation[i];
			variation += corner_cliff_variation[i] << 5;
			writer.write(variation);

			uint8_t misc = corner_cliff_texture[i] << 4;
			misc += corner_layer_height[i];
			writer.write(misc);
		}

		hierarchy.map_file_write("war3map.w3e", writer.buffer);
	}

	void render_ground(bool render_pathing, bool render_lighting, glm::vec3 light_direction, Brush* brush, PathingMap& pathing_map) const {
		// Render tiles
		ground_shader->use();

		glDisable(GL_BLEND);
		glEnable(GL_CULL_FACE);

		glUniformMatrix4fv(1, 1, GL_FALSE, &camera.projection_view[0][0]);
		glUniform1i(2, render_pathing);
		glUniform1i(3, render_lighting);
		glUniform3fv(4, 1, &light_direction.x);
		glUniform2i(7, width, height);

		if (brush) {
			glUniform2fv(5, 1, &brush->get_position()[0]);
		}

		glBindTextureUnit(17, pathing_map.texture_static);
		glBindTextureUnit(18, pathing_map.texture_dynamic);

		glUniform1i(6, brush && brush->get_mode() != Brush::Mode::selection);
		if (brush) {
			glBindTextureUnit(19, brush->brush_texture);
		}

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cliff_level_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ground_height_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ground_texture_data_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ground_exists_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ground_texture_handle_buffer);

		// Use gl_VertexID in the shader to determine square position
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (width - 1) * (height - 1));

		glEnable(GL_BLEND);

		// Render cliffs
		for (const auto& i : cliffs) {
			const size_t bl = ci(i.x, i.y);
			const size_t br = ci(i.x + 1, i.y);
			const size_t tl = ci(i.x, i.y + 1);
			const size_t tr = ci(i.x + 1, i.y + 1);

			if (corner_special_doodad[bl]) {
				continue;
			}

			const float min =
				std::min({corner_layer_height[bl], corner_layer_height[br], corner_layer_height[tl], corner_layer_height[tr]});

			cliff_meshes[i.z]->render_queue({i.x, i.y, min - 2, corner_cliff_texture[bl]});
		}

		cliff_shader->use();

		glUniformMatrix4fv(0, 1, GL_FALSE, &camera.projection_view[0][0]);
		glUniform1i(1, render_pathing);
		glUniform1i(2, render_lighting);
		glUniform3fv(3, 1, &light_direction.x);
		if (brush) {
			glUniform2fv(4, 1, &brush->get_position()[0]);
		}
		glUniform1i(5, brush && brush->get_mode() != Brush::Mode::selection);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ground_height_buffer);

		glBindTextureUnit(0, cliff_texture_array);
		glBindTextureUnit(2, pathing_map.texture_static);

		glUniform2i(7, width, height);

		if (brush) {
			glBindTextureUnit(3, brush->brush_texture);
		}
		for (const auto& i : cliff_meshes) {
			i->render();
		}
	}

	void render_water() const {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(false);

		water_shader->use();

		glUniformMatrix4fv(0, 1, GL_FALSE, &camera.projection_view[0][0]);
		glUniform4fv(1, 1, &shallow_color_min[0]);
		glUniform4fv(2, 1, &shallow_color_max[0]);
		glUniform4fv(3, 1, &deep_color_min[0]);
		glUniform4fv(4, 1, &deep_color_max[0]);
		glUniform1f(5, water_offset);
		glUniform1i(6, current_texture);
		glUniform2i(7, width, height);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cliff_level_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, water_height_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, water_exists_buffer);

		glBindTextureUnit(0, water_texture_array);

		// Use gl_VertexID in the shader to determine square position
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (width - 1) * (height - 1));

		glDepthMask(true);
	}

	void change_tileset(const std::vector<std::string>& new_tileset_ids, std::vector<int> new_to_old) {
		tileset_ids = new_tileset_ids;

		// Blight
		new_to_old.push_back(new_tileset_ids.size());

		// Map old ids to the new ids
		for (auto& ground_texture : corner_ground_texture) {
			ground_texture = new_to_old[ground_texture];
		}

		// Reload tile textures
		ground_textures.clear(); // ToDo Clear them after loading new ones?
		ground_texture_to_id.clear();
		gpu_ground_texture_handles.clear();

		for (const auto& tile_id : tileset_ids) {
			ground_textures.push_back(resource_manager
										  .load<GroundTexture>(
											  terrain_slk.data("dir", tile_id) + "/" + terrain_slk.data("file", tile_id)
											  + (hierarchy.hd ? "_diffuse.dds" : ".dds")
										  )
										  .value());
			ground_texture_to_id.emplace(tile_id, static_cast<int>(ground_textures.size() - 1));
			gpu_ground_texture_handles.push_back(ground_textures.back()->bindless_handle);
		}
		blight_texture = static_cast<int>(ground_textures.size());
		ground_texture_to_id.emplace("blight", blight_texture);
		ground_textures.push_back(resource_manager
									  .load<GroundTexture>(
										  world_edit_data.data("TileSets", std::string(1, tileset), 1)
										  + (hierarchy.hd ? "_diffuse.dds" : ".dds")
									  )
									  .value());
		gpu_ground_texture_handles.push_back(ground_textures.back()->bindless_handle);

		glNamedBufferStorage(
			ground_texture_handle_buffer,
			gpu_ground_texture_handles.size() * sizeof(GLuint64),
			gpu_ground_texture_handles.data(),
			GL_DYNAMIC_STORAGE_BIT
		);

		cliff_to_ground_texture.clear();
		for (const auto& cliff_id : cliffset_ids) {
			cliff_to_ground_texture.push_back(ground_texture_to_id[cliff_slk.data("groundtile", cliff_id)]);
		}

		update_ground_textures({0, 0, width, height});
	}

	/// The texture of the tile point which is influenced by its surroundings.
	/// Nearby cliff/ramp > blight > regular texture
	[[nodiscard]]
	uint8_t real_tile_texture(const int x, const int y) const {
		// We only need to check ourselves, to the left, bottom-left and bottom
		const size_t idx = ci(x, y);
		uint8_t a_romp = corner_romp[idx];
		uint8_t a_cliff = corner_cliff[idx];
		if (x > 0) {
			a_romp |= corner_romp[idx - 1];
			a_cliff |= corner_cliff[idx - 1];
		}
		if (y > 0) {
			a_romp |= corner_romp[idx - width];
			a_cliff |= corner_cliff[idx - width];
		}
		if (x > 0 && y > 0) {
			a_romp |= corner_romp[idx - width - 1];
			a_cliff |= corner_cliff[idx - width - 1];
		}

		if (a_romp || (a_cliff && !corner_ramp[idx])) {
			int texture = corner_cliff_texture[idx];
			// Number 15 seems to be something
			if (texture == 15) {
				texture -= 14;
			}
			return cliff_to_ground_texture[texture];
		}

		if (corner_blight[idx]) {
			return blight_texture;
		} else {
			return corner_ground_texture[idx];
		}
	}

	/// The subtexture of a groundtexture to use.
	int get_tile_variation(const int ground_texture, const int variation) const {
		if (ground_textures[ground_texture]->extended) {
			if (variation <= 15) {
				return 16 + variation;
			} else if (variation == 16) {
				return 15;
			} else {
				return 0;
			}
		} else {
			if (variation == 0) {
				return 0;
			} else {
				return 15;
			}
		}
	}

	/// Returns the height at x,y by bilinear interpolation
	/// Set water_too to true to also take the water height into account
	float interpolated_height(float x, float y, const bool water_too) const {
		x = std::clamp(x, 0.f, width - 1.01f);
		y = std::clamp(y, 0.f, height - 1.01f);

		const int ix = static_cast<int>(x);
		const int iy = static_cast<int>(y);
		const int cx = static_cast<int>(std::ceil(x));
		const int cy = static_cast<int>(std::ceil(y));

		float p1 = corner_final_ground_height(ix, iy);
		float p2 = corner_final_ground_height(cx, iy);
		float p3 = corner_final_ground_height(ix, cy);
		float p4 = corner_final_ground_height(cx, cy);

		if (water_too && corner_water[ci(ix, iy)]) {
			p1 = std::max(p1, corner_final_water_height(ix, iy));
		}

		if (water_too && corner_water[ci(cx, iy)]) {
			p2 = std::max(p2, corner_final_water_height(cx, iy));
		}

		if (water_too && corner_water[ci(ix, cy)]) {
			p3 = std::max(p3, corner_final_water_height(ix, cy));
		}

		if (water_too && corner_water[ci(cx, cy)]) {
			p4 = std::max(p4, corner_final_water_height(cx, cy));
		}

		const float xx = glm::mix(p1, p2, x - floor(x));
		const float yy = glm::mix(p3, p4, x - floor(x));
		return glm::mix(xx, yy, y - floor(y));
	}

	// Returns the y gradient in radians
	float gradient_y(float x, float y) const {
		x = std::clamp(x, 0.f, width - 1.01f);
		y = std::clamp(y, 0.f, height - 1.01f);

		const int ix = static_cast<int>(x);
		const int iy = static_cast<int>(y);
		const float bottom_left = corner_final_ground_height(ix, iy);
		const float bottom_right = corner_final_ground_height(ix + 1, iy);
		const float top_left = corner_final_ground_height(ix, iy + 1);
		const float top_right = corner_final_ground_height(ix + 1, iy + 1);

		const float bottom = glm::mix(bottom_left, bottom_right, x - glm::floor(x));
		const float top = glm::mix(top_left, top_right, x - glm::floor(x));

		return std::atan(bottom - top);
	}

	bool is_corner_ramp_entrance(const int x, const int y) const {
		if (x == width || y == height) {
			return false;
		}

		const size_t bl = ci(x, y);
		const size_t br = ci(x + 1, y);
		const size_t tl = ci(x, y + 1);
		const size_t tr = ci(x + 1, y + 1);

		return corner_ramp[bl] && corner_ramp[tl] && corner_ramp[br] && corner_ramp[tr]
			&& !(corner_layer_height[bl] == corner_layer_height[tr] && corner_layer_height[tl] == corner_layer_height[br]);
	}

	/// Constructs a minimap image with tile, cliff, and water colors. Other objects such as doodads will not be added here
	Texture minimap_image() {
		Texture new_minimap_image;

		new_minimap_image.width = width;
		new_minimap_image.height = height;
		new_minimap_image.channels = 4;
		new_minimap_image.data.resize(width * height * 4);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				glm::vec4 color;

				if (corner_cliff[ci(i, j)] || (i > 0 && corner_cliff[ci(i - 1, j)]) || (j > 0 && corner_cliff[ci(i, j - 1)])
					|| (i > 0 && j > 0 && corner_cliff[ci(i - 1, j - 1)])) {
					color = glm::vec4(128.f, 128.f, 128.f, 255.f);
				} else {
					color = ground_textures[real_tile_texture(i, j)]->minimap_color;
				}

				if (corner_water[ci(i, j)] && corner_final_water_height(i, j) > corner_final_ground_height(i, j)) {
					if (corner_final_water_height(i, j) - corner_final_ground_height(i, j) > 0.5f) {
						color *= 0.5625f;
						color += glm::vec4(0, 0, 80, 112);
					} else {
						color *= 0.75f;
						color += glm::vec4(0, 0, 48, 64);
					}
				}

				int index = (height - 1 - j) * (width * 4) + i * 4;
				new_minimap_image.data[index + 0] = color.r;
				new_minimap_image.data[index + 1] = color.g;
				new_minimap_image.data[index + 2] = color.b;
				new_minimap_image.data[index + 3] = color.a;
			}
		}

		return new_minimap_image;
	}

	void upload_ground_heights() const {
		glNamedBufferSubData(ground_height_buffer, 0, corner_height.size() * sizeof(float), corner_height.data());
	}

	void upload_corner_heights() const {
		glNamedBufferSubData(cliff_level_buffer, 0, gpu_final_ground_heights.size() * sizeof(float), gpu_final_ground_heights.data());
	}

	void upload_ground_texture() const {
		glNamedBufferSubData(
			ground_texture_data_buffer,
			0,
			gpu_ground_texture_list.size() * sizeof(glm::uvec4),
			gpu_ground_texture_list.data()
		);
	}

	void upload_ground_exists() const {
		glNamedBufferSubData(ground_exists_buffer, 0, gpu_ground_exists_data.size() * sizeof(uint32_t), gpu_ground_exists_data.data());
	}

	void upload_water_exists() const {
		glNamedBufferSubData(water_exists_buffer, 0, gpu_water_exists_data.size() * sizeof(uint32_t), gpu_water_exists_data.data());
	}

	void upload_water_heights() const {
		glNamedBufferSubData(water_height_buffer, 0, corner_water_height.size() * sizeof(float), corner_water_height.data());
	}

	void update_ground_heights(const QRect& area) {
		// Set base ground heights for all corners in area
		for (int j = area.y(); j < area.y() + area.height(); j++) {
			for (int i = area.x(); i < area.x() + area.width(); i++) {
				const size_t idx = ci(i, j);
				gpu_final_ground_heights[idx] = corner_height[idx] + corner_layer_height[idx] - 2.0f;
			}
		}

		// For each ramp entrance tile overlapping the area, set base + 0.5 for corners at the base level.
		// Uses assignment so corners shared by multiple ramp tiles are written idempotently.
		const QRect tile_area = area.adjusted(-1, -1, 0, 0).intersected({0, 0, width - 1, height - 1});
		for (int j = tile_area.y(); j < tile_area.y() + tile_area.height(); j++) {
			for (int i = tile_area.x(); i < tile_area.x() + tile_area.width(); i++) {
				const size_t bl = ci(i, j);
				const size_t br = bl + 1;
				const size_t tl = bl + width;
				const size_t tr = bl + width + 1;

				if (!(corner_ramp[bl] && corner_ramp[tl] && corner_ramp[br] && corner_ramp[tr])) {
					continue;
				}

				if (corner_layer_height[bl] == corner_layer_height[tr] && corner_layer_height[tl] == corner_layer_height[br]) {
					continue;
				}

				// Multiple iterations might set the same index so needs to be idempotent
				const int base =
					std::min({corner_layer_height[bl], corner_layer_height[br], corner_layer_height[tl], corner_layer_height[tr]});
				if (corner_layer_height[bl] == base) {
					gpu_final_ground_heights[bl] = corner_height[bl] + corner_layer_height[bl] - 2.0f + 0.5f;
				}
				if (corner_layer_height[br] == base) {
					gpu_final_ground_heights[br] = corner_height[br] + corner_layer_height[br] - 2.0f + 0.5f;
				}
				if (corner_layer_height[tl] == base) {
					gpu_final_ground_heights[tl] = corner_height[tl] + corner_layer_height[tl] - 2.0f + 0.5f;
				}
				if (corner_layer_height[tr] == base) {
					gpu_final_ground_heights[tr] = corner_height[tr] + corner_layer_height[tr] - 2.0f + 0.5f;
				}
			}
		}

		upload_ground_heights();
		upload_corner_heights();
	}

	/// Updates the ground texture variation information and uploads it to the GPU
	/// Make sure update_cliff_meshes() is up to date on the target area
	void update_ground_textures(const QRect& area) {
		const QRect update_area = area.adjusted(-2, -2, 2, 2).intersected({0, 0, width, height});

		const int x0 = update_area.x();
		const int y0 = update_area.y();
		const int scratch_width = update_area.width();
		const int scratch_height = update_area.height();

		std::vector<uint8_t> scratch(scratch_width * scratch_height);
		for (int j = 0; j < scratch_height; ++j) {
			for (int i = 0; i < scratch_width; ++i) {
				scratch[j * scratch_width + i] = real_tile_texture(x0 + i, y0 + j);
			}
		}

		for (int j = 0; j < scratch_height - 1; ++j) {
			for (int i = 0; i < scratch_width - 1; ++i) {
				const uint8_t bottom_left = scratch[j * scratch_width + i];
				const uint8_t bottom_right = scratch[j * scratch_width + (i + 1)];
				const uint8_t top_left = scratch[(j + 1) * scratch_width + i];
				const uint8_t top_right = scratch[(j + 1) * scratch_width + (i + 1)];

				uint16_t u[4] = {bottom_left, bottom_right, top_right, top_left};
				std::sort(u, u + 4);
				uint16_t* last = std::unique(u, u + 4);

				const int tx = x0 + i;
				const int ty = y0 + j;

				glm::uvec4 out(0xFFFFu);
				out.x = u[0] | get_tile_variation(u[0], corner_ground_variation[ci(tx, ty)]) << 16;

				for (int k = 1; k < std::distance(u, last); ++k) {
					uint32_t mask = 0;
					mask |= static_cast<uint32_t>(bottom_right == u[k]);
					mask |= static_cast<uint32_t>(bottom_left == u[k]) << 1;
					mask |= static_cast<uint32_t>(top_right == u[k]) << 2;
					mask |= static_cast<uint32_t>(top_left == u[k]) << 3;
					out[k] = u[k] | mask << 16;
				}

				gpu_ground_texture_list[ty * (width - 1) + tx] = out;
			}
		}

		upload_ground_texture();
	}

	void update_ground_exists(const QRect& area) {
		const QRect update_area = area.adjusted(-1, -1, 1, 1).intersected({0, 0, width - 1, height - 1});

		for (int j = update_area.top(); j <= update_area.bottom(); j++) {
			for (int i = update_area.left(); i <= update_area.right(); i++) {
				const size_t idx = ci(i, j);
				gpu_ground_exists_data[j * (width - 1) + i] =
					!(((corner_cliff[idx] || corner_romp[idx]) && !is_corner_ramp_entrance(i, j)) || corner_special_doodad[idx]);
			}
		}

		upload_ground_exists();
	}

	/// Updates and uploads the water data for the GPU
	void update_water(const QRect& area) {
		for (int i = area.x(); i < area.x() + area.width(); i++) {
			for (int j = area.y(); j < area.y() + area.height(); j++) {
				const size_t idx = ci(i, j);
				gpu_water_exists_data[j * width + i] = corner_water[idx];
			}
		}
		upload_water_exists();
		upload_water_heights();
	}

	/// Updates the cliff and ramp meshes for an area
	void update_cliff_meshes(const QRect& area) {
		// Remove all existing cliff meshes in area
		std::erase_if(cliffs, [&](const glm::ivec3& p) {
			return area.contains(p.x, p.y);
		});

		for (int i = area.x(); i < area.right(); i++) {
			for (int j = area.y(); j < area.bottom(); j++) {
				corner_romp[ci(i, j)] = false;
			}
		}

		QRect ramp_area = area.adjusted(-2, -2, 2, 2).intersected({0, 0, width, height});

		// Add new cliff meshes
		for (int i = ramp_area.x(); i < ramp_area.right(); i++) {
			for (int j = ramp_area.y(); j < ramp_area.bottom(); j++) {
				const size_t bl = ci(i, j);
				const size_t br = ci(i + 1, j);
				const size_t tl = ci(i, j + 1);
				const size_t tr = ci(i + 1, j + 1);

				// Vertical ramps
				if (j < height - 2) {
					const size_t ttl = ci(i, j + 2);
					const size_t ttr = ci(i + 1, j + 2);
					const int ae = std::min(corner_layer_height[bl], corner_layer_height[ttl]);
					const int cf = std::min(corner_layer_height[br], corner_layer_height[ttr]);

					if (corner_layer_height[tl] == ae && corner_layer_height[tr] == cf) {
						const int base = std::min(ae, cf);
						if (corner_ramp[bl] == corner_ramp[tl] && corner_ramp[bl] == corner_ramp[ttl] && corner_ramp[br] == corner_ramp[tr]
							&& corner_ramp[br] == corner_ramp[ttr] && corner_ramp[bl] != corner_ramp[br]) {
							std::string file_name = ""s
								+ char((corner_ramp[ttl] ? 'L' : 'A') + (corner_layer_height[ttl] - base) * (corner_ramp[ttl] ? -4 : 1))
								+ char((corner_ramp[ttr] ? 'L' : 'A') + (corner_layer_height[ttr] - base) * (corner_ramp[ttr] ? -4 : 1))
								+ char((corner_ramp[br] ? 'L' : 'A') + (corner_layer_height[br] - base) * (corner_ramp[br] ? -4 : 1))
								+ char((corner_ramp[bl] ? 'L' : 'A') + (corner_layer_height[bl] - base) * (corner_ramp[bl] ? -4 : 1));

							file_name = "doodads/terrain/clifftrans/clifftrans" + file_name + "0.mdx";
							if (hierarchy.file_exists(file_name)) {
								if (!path_to_cliff.contains(file_name)) {
									cliff_meshes.push_back(resource_manager.load<CliffMesh>(file_name).value());
									path_to_cliff.emplace(file_name, static_cast<int>(cliff_meshes.size()) - 1);
								}

								cliffs.emplace_back(i, j, path_to_cliff[file_name]);
								corner_romp[bl] = true;
								corner_romp[tl] = true;

								continue;
							}
						}
					}
				}

				// Horizontal ramps
				if (i < width - 2) {
					const size_t brr = ci(i + 2, j);
					const size_t trr = ci(i + 2, j + 1);
					const int ae = std::min(corner_layer_height[bl], corner_layer_height[brr]);
					const int bf = std::min(corner_layer_height[tl], corner_layer_height[trr]);

					if (corner_layer_height[br] == ae && corner_layer_height[tr] == bf) {
						const int base = std::min(ae, bf);
						if (corner_ramp[bl] == corner_ramp[br] && corner_ramp[bl] == corner_ramp[brr] && corner_ramp[tl] == corner_ramp[tr]
							&& corner_ramp[tl] == corner_ramp[trr] && corner_ramp[bl] != corner_ramp[tl]) {
							std::string file_name = ""s
								+ char((corner_ramp[tl] ? 'L' : 'A') + (corner_layer_height[tl] - base) * (corner_ramp[tl] ? -4 : 1))
								+ char((corner_ramp[trr] ? 'L' : 'A') + (corner_layer_height[trr] - base) * (corner_ramp[trr] ? -4 : 1))
								+ char((corner_ramp[brr] ? 'L' : 'A') + (corner_layer_height[brr] - base) * (corner_ramp[brr] ? -4 : 1))
								+ char((corner_ramp[bl] ? 'L' : 'A') + (corner_layer_height[bl] - base) * (corner_ramp[bl] ? -4 : 1));

							file_name = "doodads/terrain/clifftrans/clifftrans" + file_name + "0.mdx";
							if (hierarchy.file_exists(file_name)) {
								if (!path_to_cliff.contains(file_name)) {
									cliff_meshes.push_back(resource_manager.load<CliffMesh>(file_name).value());
									path_to_cliff.emplace(file_name, static_cast<int>(cliff_meshes.size()) - 1);
								}

								cliffs.emplace_back(i, j, path_to_cliff[file_name]);
								corner_romp[bl] = true;
								corner_romp[br] = true;

								continue;
							}
						}
					}
				}

				if (!corner_cliff[bl] || corner_romp[bl]) {
					continue;
				}

				if (is_corner_ramp_entrance(i, j)) {
					continue;
				}

				const int base =
					std::min({corner_layer_height[bl], corner_layer_height[br], corner_layer_height[tl], corner_layer_height[tr]});

				// Cliff model path
				std::string file_name = ""s + static_cast<char>('A' + corner_layer_height[tl] - base)
					+ static_cast<char>('A' + corner_layer_height[tr] - base) + static_cast<char>('A' + corner_layer_height[br] - base)
					+ static_cast<char>('A' + corner_layer_height[bl] - base);

				if (file_name == "AAAA") {
					continue;
				}

				// Clamp to within max variations
				file_name += std::to_string(std::clamp(corner_cliff_variation[bl], static_cast<uint8_t>(0), cliff_variations[file_name]));

				cliffs.emplace_back(i, j, path_to_cliff[file_name]);
			}
		}

		update_ground_exists(ramp_area);
	}

	/// Computes the terrain pathing flags for the target cell on the **PATHING** map
	/// Takes cliffs, blight, water, terrain textures and boundaries into account
	uint8_t get_terrain_pathing(size_t i, size_t j, bool tile_pathing, bool cliff_pathing, bool water_pathing) {
		// map pathing cell to corner
		const size_t cx = i / 4;
		const size_t cy = j / 4;

		const size_t bl_idx = ci(cx, cy);

		uint8_t mask = 0;

		// take terrain texture into account (from the closest corner)
		const int x = static_cast<int>(std::round(i / 4.0));
		const int y = static_cast<int>(std::round(j / 4.0));
		const size_t closest_idx = ci(x, y);

		if (tile_pathing) {
			mask = pathing_options[tileset_ids[corner_ground_texture[closest_idx]]].mask();
		}

		// cliffs are unbuildable and unwalkable
		if (cliff_pathing) {
			const bool is_cliff = corner_cliff[bl_idx] || corner_romp[bl_idx];

			if (is_cliff) {
				// check if its a full ramp
				const bool is_ramp = corner_ramp[ci(cx, cy)] && corner_ramp[ci(cx + 1, cy)] && corner_ramp[ci(cx, cy + 1)]
					&& corner_ramp[ci(cx + 1, cy + 1)];

				if (!is_ramp) {
					mask |= PathingMap::unbuildable | PathingMap::unwalkable;
				}
			}
		}

		// take blight into account
		if (corner_blight[closest_idx]) {
			mask |= PathingMap::blight;
		}

		// take water into account
		if (water_pathing && corner_water[closest_idx]) {
			// apply water mask
			mask |= PathingMap::water;

			if (corner_final_water_height(x, y) > corner_final_ground_height(x, y) + 0.40) {
				// deep water is unwalkable and unbuildable
				mask |= PathingMap::unbuildable | PathingMap::unwalkable;
			} else if (corner_final_water_height(x, y) > corner_final_ground_height(x, y)) {
				// shallow water is unbuildable
				mask |= PathingMap::unbuildable;
			}
		}

		// boundaries and map edges are unwalkable, unflyable and unbuildable¸
		// NOTE: game handles corners/boundaries differently
		// if the bottom-left corner is a boundary, then the entire 4x4 cell is considered a boundary
		if (corner_map_edge[bl_idx] || corner_boundary[bl_idx]) {
			mask |= PathingMap::unbuildable | PathingMap::unflyable | PathingMap::unwalkable;
		}

		return mask;
	}

	/// Updates the map_edge flags (shadows on the edges of the map)
	void set_unplayable_boundaries(int unplayable_left, int unplayable_right, int unplayable_top, int unplayable_bottom) {
		// places "shadows" on the edges of the map
		for (size_t i = 0; i < width; i++) {
			for (size_t j = 0; j < height; j++) {
				corner_map_edge[ci(i, j)] =
					i < unplayable_left || i >= width - unplayable_right || j < unplayable_bottom || j >= height - unplayable_top;
			}
		}

		emit minimap_changed(minimap_image());
	}

	/// Resizes the terrain by expanding/shrinking it from all four sides
	void resize(int delta_left, int delta_right, int delta_top, int delta_bottom, Physics& physics) {
		const int new_width = width + delta_left + delta_right;
		const int new_height = height + delta_top + delta_bottom;

		// width/height must be at least 33
		// in reforged, map size is not constrained to multiples of 32
		if (new_width < 33 || new_height < 33) {
			return;
		}

		// maximum map size supported by wc3 is 481 x 481
		if (new_width > 481 || new_height > 481) {
			return;
		}

		// create and setup
		resize_corner_data(delta_left, delta_right, delta_top, delta_bottom, new_width, new_height);

		// update terrain object
		width = new_width;
		height = new_height;
		offset.x -= delta_left * 128;
		offset.y -= delta_bottom * 128;

		re_render(physics);

		emit minimap_changed(minimap_image());
	}

	void update_minimap() {
		emit minimap_changed(minimap_image());
	}

  private:
	void resize_corner_data(
		const int delta_left,
		const int delta_right,
		const int delta_top,
		const int delta_bottom,
		const int new_width,
		const int new_height
	) {
		const int new_total = new_width * new_height;
		// Helper lambda to compute new index
		auto new_ci = [new_width](const size_t x, const size_t y) -> size_t {
			return y * new_width + x;
		};

		// Allocate new SoA arrays with defaults
		std::vector<float> new_height_arr(new_total, 0.f);
		std::vector<float> new_water_height(new_total, 0.f);
		std::vector<uint8_t> new_ground_texture(new_total, 0);
		std::vector<uint8_t> new_ground_variation(new_total, 0);
		std::vector<uint8_t> new_cliff_variation(new_total, 0);
		std::vector<uint8_t> new_cliff_texture(new_total, 15);
		std::vector<uint8_t> new_layer_height(new_total, 2);
		std::vector<uint8_t> new_map_edge(new_total, 0);
		std::vector<uint8_t> new_ramp(new_total, 0);
		std::vector<uint8_t> new_blight(new_total, 0);
		std::vector<uint8_t> new_water(new_total, 0);
		std::vector<uint8_t> new_boundary(new_total, 0);
		std::vector<uint8_t> new_cliff(new_total, 0);
		std::vector<uint8_t> new_romp(new_total, 0);
		std::vector<uint8_t> new_special_doodad(new_total, 0);

		// check if all old corners would be removed
		const int old_width_remaining = width + std::min(0, delta_left) + std::min(0, delta_right);
		const int old_height_remaining = height + std::min(0, delta_bottom) + std::min(0, delta_top);
		if (old_width_remaining <= 0 || old_height_remaining <= 0) {
			// all old corners deleted, fill with default
			for (int i = 0; i < new_width; i++) {
				for (int j = 0; j < new_height; j++) {
					const size_t idx = new_ci(i, j);
					new_ground_variation[idx] = random_ground_variation();
					new_map_edge[idx] = (i == 0 || i == new_width - 1 || j == 0 || j == new_height - 1);
				}
			}
		} else {
			// fill by mapping from old corners
			for (int i = 0; i < new_width; i++) {
				for (int j = 0; j < new_height; j++) {
					int old_i = i - delta_left;
					int old_j = j - delta_bottom;

					const bool is_old_corner = (old_i >= 0 && old_i < width && old_j >= 0 && old_j < height);

					old_i = std::clamp(old_i, 0, width - 1);
					old_j = std::clamp(old_j, 0, height - 1);

					const size_t new_idx = new_ci(i, j);
					const size_t old_idx = ci(old_i, old_j);

					new_height_arr[new_idx] = corner_height[old_idx];
					new_water_height[new_idx] = corner_water_height[old_idx];
					new_ground_texture[new_idx] = corner_ground_texture[old_idx];
					new_ground_variation[new_idx] = corner_ground_variation[old_idx];
					new_cliff_variation[new_idx] = corner_cliff_variation[old_idx];
					new_cliff_texture[new_idx] = corner_cliff_texture[old_idx];
					new_layer_height[new_idx] = corner_layer_height[old_idx];
					new_map_edge[new_idx] = corner_map_edge[old_idx];
					new_ramp[new_idx] = corner_ramp[old_idx];
					new_blight[new_idx] = corner_blight[old_idx];
					new_water[new_idx] = corner_water[old_idx];
					new_boundary[new_idx] = corner_boundary[old_idx];
					new_cliff[new_idx] = corner_cliff[old_idx];
					new_romp[new_idx] = corner_romp[old_idx];
					new_special_doodad[new_idx] = corner_special_doodad[old_idx];

					if (!is_old_corner) {
						new_special_doodad[new_idx] = false;
						new_ground_variation[new_idx] = random_ground_variation();
					}
				}
			}
		}

		corner_height = std::move(new_height_arr);
		corner_water_height = std::move(new_water_height);
		corner_ground_texture = std::move(new_ground_texture);
		corner_ground_variation = std::move(new_ground_variation);
		corner_cliff_variation = std::move(new_cliff_variation);
		corner_cliff_texture = std::move(new_cliff_texture);
		corner_layer_height = std::move(new_layer_height);
		corner_map_edge = std::move(new_map_edge);
		corner_ramp = std::move(new_ramp);
		corner_blight = std::move(new_blight);
		corner_water = std::move(new_water);
		corner_boundary = std::move(new_boundary);
		corner_cliff = std::move(new_cliff);
		corner_romp = std::move(new_romp);
		corner_special_doodad = std::move(new_special_doodad);
	}

	void re_render(Physics& physics) {
		// clear old buffers
		glDeleteBuffers(1, &ground_height_buffer);
		glDeleteBuffers(1, &cliff_level_buffer);
		glDeleteBuffers(1, &water_height_buffer);
		glDeleteBuffers(1, &ground_texture_data_buffer);
		glDeleteBuffers(1, &ground_exists_buffer);
		glDeleteBuffers(1, &water_exists_buffer);

		// clear all cliffs (this prevents out of bounds error when shrinking terrain)
		cliffs.clear();

		// clear romp flags on all corners
		std::fill(corner_romp.begin(), corner_romp.end(), 0);

		// determine cliff flags for all corners
		compute_cliff_flags();

		// resize and recreate GPU buffers
		setup_GPU_buffers();

		// update everything else
		update_ground_heights({0, 0, width, height});
		update_ground_textures({0, 0, width, height});
		update_cliff_meshes({0, 0, width - 1, height - 1});
		update_water({0, 0, width, height});

		// update physics collision shape
		physics.dynamicsWorld->removeRigidBody(collision_body);
		delete collision_body;
		delete collision_shape;
		setup_collision_shape(physics);
	}

	void compute_cliff_flags() {
		for (int i = 0; i < width - 1; i++) {
			for (int j = 0; j < height - 1; j++) {
				const size_t bl = ci(i, j);
				const size_t br = ci(i + 1, j);
				const size_t tl = ci(i, j + 1);
				const size_t tr = ci(i + 1, j + 1);

				corner_cliff[bl] = corner_layer_height[bl] != corner_layer_height[br] || corner_layer_height[bl] != corner_layer_height[tl]
					|| corner_layer_height[bl] != corner_layer_height[tr];
			}
		}
	}

	void setup_GPU_buffers() {
		// setup derived GPU vectors
		gpu_final_ground_heights.resize(width * height);
		gpu_ground_texture_list.resize((width - 1) * (height - 1));
		gpu_ground_exists_data.resize(width * height);
		gpu_water_exists_data.resize(width * height);

		// ground buffers
		glCreateBuffers(1, &ground_height_buffer);
		glNamedBufferStorage(ground_height_buffer, width * height * sizeof(float), nullptr, GL_DYNAMIC_STORAGE_BIT);
		glCreateBuffers(1, &cliff_level_buffer);
		glNamedBufferStorage(cliff_level_buffer, width * height * sizeof(float), nullptr, GL_DYNAMIC_STORAGE_BIT);
		glCreateBuffers(1, &ground_texture_data_buffer);
		glNamedBufferStorage(ground_texture_data_buffer, (width - 1) * (height - 1) * sizeof(glm::uvec4), nullptr, GL_DYNAMIC_STORAGE_BIT);
		glCreateBuffers(1, &ground_exists_buffer);
		glNamedBufferStorage(ground_exists_buffer, width * height * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);

		// water buffers
		glCreateBuffers(1, &water_height_buffer);
		glNamedBufferStorage(water_height_buffer, width * height * sizeof(float), nullptr, GL_DYNAMIC_STORAGE_BIT);
		glCreateBuffers(1, &water_exists_buffer);
		glNamedBufferStorage(water_exists_buffer, width * height * sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);
	}

	void setup_collision_shape(const Physics& physics) {
		collision_shape =
			new btHeightfieldTerrainShape(width, height, gpu_final_ground_heights.data(), 0, -16.f, 16.f, 2, PHY_FLOAT, false);
		collision_body = new btRigidBody(0, new btDefaultMotionState(), collision_shape);
		collision_body->getWorldTransform().setOrigin(btVector3(width / 2.f - 0.5f, height / 2.f - 0.5f, 0.f));
		collision_body->setCollisionFlags(collision_body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
		physics.dynamicsWorld->addRigidBody(collision_body, 32, 32);
	}

  signals:
	void minimap_changed(Texture minimap);
};

#include "terrain.moc"
