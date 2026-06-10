export module Regions;

import std;
import BinaryReader;
import BinaryWriter;
import Hierarchy;
import <glad/glad.h>;
import <glm/glm.hpp>;

/// The thickness of a drawn region border in world space (one tile = 1)
/// Must match the `border` constant in terrain.frag and cliff.frag
export constexpr float region_border_size() {
	return 0.1f;
}

/// Darker pastel colors that new regions cycle through
export constexpr std::array<glm::u8vec3, 10> region_preset_colors = {{
	{ 177, 84, 84 },
	{ 185, 122, 86 },
	{ 181, 157, 84 },
	{ 122, 158, 90 },
	{ 84, 158, 146 },
	{ 84, 122, 168 },
	{ 122, 98, 168 },
	{ 164, 92, 146 },
	{ 180, 100, 124 },
	{ 110, 122, 138 },
}};

export struct Region {
	float left;
	float right;
	float top;
	float bottom;
	std::string name;
	int creation_number;
	std::string weather_id;
	std::string ambient_id;
	glm::u8vec3 color = {255, 0, 0};
};

export class Regions {
	static constexpr int write_version = 5;

	struct RegionRenderData {
		glm::vec4 rect; // left, bottom, right, top
		glm::vec4 color; // rgb + selected flag
	};

  public:
	std::vector<Region> regions;

	GLuint render_buffer = 0;

	/// Uploads the region rectangles/colors to the storage buffer
	/// sampled by the terrain/cliff shaders to draw them onto the terrain
	void update_render_buffer(const std::unordered_set<Region*>* selections) {
		if (!render_buffer) {
			glCreateBuffers(1, &render_buffer);
		}

		std::vector<RegionRenderData> data;
		data.reserve(regions.size());
		for (auto& region : regions) {
			float left = std::min(region.left, region.right);
			float right = std::max(region.left, region.right);
			float bottom = std::min(region.bottom, region.top);
			float top = std::max(region.bottom, region.top);

			// Draw at least one cell so degenerate regions (e.g. mid resize/creation) stay visible
			if (right - left < 0.25f) {
				const float center = (left + right) / 2.f;
				left = center - 0.125f;
				right = center + 0.125f;
			}
			if (top - bottom < 0.25f) {
				const float center = (bottom + top) / 2.f;
				bottom = center - 0.125f;
				top = center + 0.125f;
			}

			const bool selected = selections && selections->contains(&region);
			data.push_back({
				.rect = { left, bottom, right, top },
				.color = { glm::vec3(region.color) / 255.f, selected ? 1.f : 0.f },
			});
		}

		glNamedBufferData(render_buffer, data.size() * sizeof(RegionRenderData), data.data(), GL_DYNAMIC_DRAW);
	}

	bool load(float terrain_offset_x, float terrain_offset_y) {
		BinaryReader reader = hierarchy.map_file_read("war3map.w3r").value();

		const int version = reader.read<uint32_t>();
		if (version != 5) {
			std::cout << "Unknown Regions file version. Attempting to load, but may crash.";
		}

		regions.resize(reader.read<uint32_t>());
		for (auto& i : regions) {
			i.left = (reader.read<float>() - terrain_offset_x) / 128.f;
			i.bottom = (reader.read<float>() - terrain_offset_y) / 128.f;
			i.right = (reader.read<float>() - terrain_offset_x) / 128.f;
			i.top = (reader.read<float>() - terrain_offset_y) / 128.f;

			i.name = reader.read_c_string();
			i.creation_number = reader.read<int>();
			i.weather_id = reader.read_string(4);
			i.ambient_id = reader.read_c_string();
			const auto color = reader.read<glm::u8vec3>();
			i.color = {color.b, color.g, color.r}; // BGR to RGB
			reader.advance(1);
		}

		return true;
	}

	void save(const float terrain_offset_x, const float terrain_offset_y) const {
		BinaryWriter writer;
		writer.write<uint32_t>(write_version);
		writer.write<uint32_t>(regions.size());

		for (const auto& i : regions) {
			writer.write<float>(i.left * 128.f + terrain_offset_x);
			writer.write<float>(i.bottom * 128.f + terrain_offset_y);
			writer.write<float>(i.right * 128.f + terrain_offset_x);
			writer.write<float>(i.top * 128.f + terrain_offset_y);

			writer.write_c_string(i.name);
			writer.write<int>(i.creation_number);
			writer.write_c_string_padded(i.weather_id, 4);
			writer.write_c_string(i.ambient_id);
			writer.write(glm::u8vec3(i.color.b, i.color.g, i.color.r));
			writer.write<uint8_t>(0xFF);
		}

		hierarchy.map_file_write("war3map.w3r", writer.buffer);
	}

	int get_unique_creation_number() const {
		int number = 0;
		for (const auto& i : regions) {
			number = std::max(number, i.creation_number + 1);
		}
		return number;
	}

	std::string get_unique_name() const {
		for (int counter = 0;; counter++) {
			std::string name = std::format("Region {:03}", counter);
			if (!std::ranges::any_of(regions, [&](const Region& region) { return region.name == name; })) {
				return name;
			}
		}
	}

	void remove_region(Region* region) {
		const auto iterator = regions.begin() + std::distance(regions.data(), region);
		regions.erase(iterator);
	}

	void remove_regions(const std::unordered_set<Region*>& list) {
		std::erase_if(regions, [&](Region& region) {
			return list.contains(&region);
		});
	}
};
