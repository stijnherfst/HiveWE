#include "doodads.h"

#include <print>
#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "globals.h"
#include <map_global.h>


import BinaryWriter;
import Hierarchy;

void Doodad::init(std::string id, std::shared_ptr<SkinnedMesh> mesh) {
	this->id = id;
	this->skin_id = id;
	this->mesh = mesh;

	skeleton = SkeletalModelInstance(mesh->model);
	// Get pathing map
	const bool is_doodad = doodads_slk.row_headers.contains(id);
	const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

	pathing.reset();
	const std::string pathing_texture_path = slk.data("pathtex", id);
	if (hierarchy.file_exists(pathing_texture_path)) {
		pathing = resource_manager.load<PathingTexture>(pathing_texture_path);
	}
	update();
}

void Doodad::update() {
	float base_scale = 1.f;
	float max_roll;
	float max_pitch;
	if (doodads_slk.row_headers.contains(id)) {
		color.r = doodads_slk.data<float>("vertr" + std::to_string(variation + 1), id) / 255.f;
		color.g = doodads_slk.data<float>("vertg" + std::to_string(variation + 1), id) / 255.f;
		color.b = doodads_slk.data<float>("vertb" + std::to_string(variation + 1), id) / 255.f;
		max_roll = doodads_slk.data<float>("maxroll", id);
		max_pitch = doodads_slk.data<float>("maxpitch", id);
		base_scale = doodads_slk.data<float>("defscale", id);
	} else {
		color.r = destructibles_slk.data<float>("colorr", id) / 255.f;
		color.g = destructibles_slk.data<float>("colorg", id) / 255.f;
		color.b = destructibles_slk.data<float>("colorb", id) / 255.f;
		max_roll = destructibles_slk.data<float>("maxroll", id);
		max_pitch = destructibles_slk.data<float>("maxpitch", id);
	}

	glm::quat rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));

	constexpr float SAMPLE_RADIUS = 32.f / 128.f;

	float pitch = 0.f;
	if (max_pitch < 0.f) {
		pitch = max_pitch;
	} else if (max_pitch > 0.f) {
		float forward_x = position.x + (SAMPLE_RADIUS * std::cos(angle));
		float forward_y = position.y + (SAMPLE_RADIUS * std::sin(angle));
		float backward_x = position.x - (SAMPLE_RADIUS * std::cos(angle));
		float backward_y = position.y - (SAMPLE_RADIUS * std::sin(angle));

		float height1 = map->terrain.interpolated_height(backward_x, backward_y, false);
		float height2 = map->terrain.interpolated_height(forward_x, forward_y, false);

		pitch = std::clamp(std::atan2(height2 - height1, SAMPLE_RADIUS * 2.f), -pitch, pitch);
	}
	rotation *= glm::angleAxis(-pitch, glm::vec3(0, 1, 0));

	float roll = 0.f;
	if (max_roll < 0.f) {
		roll = max_roll;
	} else if (max_roll > 0.f) {
		float left_of_angle = angle + (3.1415926535 / 2.0);
		float forward_x = position.x + (SAMPLE_RADIUS * std::cos(left_of_angle));
		float forward_y = position.y + (SAMPLE_RADIUS * std::sin(left_of_angle));
		float backward_x = position.x - (SAMPLE_RADIUS * std::cos(left_of_angle));
		float backward_y = position.y - (SAMPLE_RADIUS * std::sin(left_of_angle));

		float height1 = map->terrain.interpolated_height(backward_x, backward_y, false);
		float height2 = map->terrain.interpolated_height(forward_x, forward_y, false);

		roll = std::clamp(atan2(height2 - height1, SAMPLE_RADIUS * 2.f), -roll, roll);
	}
	rotation *= glm::angleAxis(roll, glm::vec3(1, 0, 0));

	skeleton.update_location(position, rotation, (base_scale * scale) / 128.f);
}

glm::vec2 Doodad::acceptable_position(glm::vec2 position, std::shared_ptr<PathingTexture> pathing, float rotation, bool force_grid_aligned) {
	if (!pathing) {
		if (force_grid_aligned) {
			return glm::round(position * 2.f) * 0.5f;
		} else {
			return position;
		}
	}

	auto rotated_pathing_width = pathing->width;
	auto rotated_pathing_height = pathing->height;

	if (static_cast<uint32_t>(glm::round(glm::degrees(rotation))) % 180 == 0) {
		rotated_pathing_width = pathing->height;
		rotated_pathing_height = pathing->width;
	}

	glm::vec2 extra_offset(0.0f);
	if (rotated_pathing_width % 4 != 0) {
		extra_offset.x = 0.25f;
	}

	if (rotated_pathing_height % 4 != 0) {
		extra_offset.y = 0.25f;
	}

	return glm::round((position + extra_offset) * 2.f) * 0.5f - extra_offset;
}

float Doodad::acceptable_angle(std::string_view id, std::shared_ptr<PathingTexture> pathing, float current_angle, float target_angle) {
	float fixed_rotation = 0.0;
	if (doodads_slk.row_headers.contains(id)) {
		fixed_rotation = doodads_slk.data<float>("fixedrot", id);
	} else {
		fixed_rotation = destructibles_slk.data<float>("fixedrot", id);
	}

	// Negative values indicate free rotation, positive is a fixed angle
	if (fixed_rotation >= 0.0) {
		return glm::radians(fixed_rotation);
	}

	if (pathing) {
		if (pathing->width == pathing->height && pathing->homogeneous) {
			return target_angle;
		} else {
			return (static_cast<int>((target_angle + glm::pi<float>() * 0.25f) / (glm::pi<float>() * 0.5f)) % 4) * glm::pi<float>() * 0.5f;
		}
	} else {
		return target_angle;
	}
}

bool Doodads::load() {
	BinaryReader reader = hierarchy.map_file_read("war3map.doo");

	const std::string magic_number = reader.read_string(4);
	if (magic_number != "W3do") {
		std::println("Invalid war3map.doo file: Magic number is not W3do");
		return false;
	}
	const uint32_t version = reader.read<uint32_t>();
	if (version != 7 && version != 8) {
		std::println("Unknown war3map.doo version: {} Attempting to load but may crash\nPlease send this map to eejin\n", version);
	}

	// Subversion
	const uint32_t subversion = reader.read<uint32_t>();
	// ToDO check subversion

	Doodad::auto_increment = 0;
	doodads.resize(reader.read<uint32_t>());
	for (auto&& i : doodads) {
		i.id = reader.read_string(4);
		i.variation = reader.read<uint32_t>();
		i.position = (reader.read<glm::vec3>() - glm::vec3(map->terrain.offset, 0)) / 128.f;
		i.angle = reader.read<float>();
		i.scale = reader.read<glm::vec3>();

		if (map->info.game_version_major * 100 + map->info.game_version_minor >= 132) {
			i.skin_id = reader.read_string(4);
		} else {
			i.skin_id = i.id;
		}

		i.state = static_cast<Doodad::State>(reader.read<uint8_t>());
		i.life = reader.read<uint8_t>();

		if (version >= 8) {
			i.item_table_pointer = reader.read<int32_t>();
			i.item_sets.resize(reader.read<uint32_t>());
			for (auto&& j : i.item_sets) {
				j.items.resize(reader.read<uint32_t>());
				for (auto& [chance, id] : j.items) {
					id = reader.read_string(4);
					chance = reader.read<uint32_t>();
				}
			}
		}

		i.creation_number = reader.read<uint32_t>();
		Doodad::auto_increment = std::max(Doodad::auto_increment, i.creation_number);
	}

	// Terrain Doodads
	const int special_format_version = reader.read<uint32_t>();

	special_doodads.resize(reader.read<uint32_t>());
	for (auto&& i : special_doodads) {
		i.id = reader.read_string(4);
		i.variation = reader.read<uint32_t>();
		i.position = glm::ivec3(reader.read<glm::ivec2>(), 0);
		i.old_position = i.position;
	}

	return true;
}

void Doodads::save() const {
	BinaryWriter writer;
	writer.write_string("W3do");
	writer.write<uint32_t>(write_version);
	writer.write<uint32_t>(write_subversion);

	writer.write<uint32_t>(doodads.size());
	for (auto&& i : doodads) {
		writer.write_string(i.id);
		writer.write<uint32_t>(i.variation);
		writer.write<glm::vec3>(i.position * 128.f + glm::vec3(map->terrain.offset, 0));
		writer.write<float>(i.angle);
		writer.write<glm::vec3>(i.scale);

		writer.write_string(i.skin_id);

		writer.write<uint8_t>(static_cast<int>(i.state));
		writer.write<uint8_t>(i.life);

		writer.write<int32_t>(i.item_table_pointer);
		writer.write<uint32_t>(i.item_sets.size());
		for (auto&& j : i.item_sets) {
			writer.write<uint32_t>(j.items.size());
			for (const auto& [chance, id] : j.items) {
				writer.write_string(id);
				writer.write<uint32_t>(chance);
			}
		}

		writer.write<uint32_t>(i.creation_number);
	}

	writer.write<uint32_t>(write_special_version);

	writer.write<uint32_t>(special_doodads.size());
	for (auto&& i : special_doodads) {
		writer.write_string(i.id);
		writer.write<uint32_t>(i.variation);
		writer.write<glm::ivec2>(glm::ivec2(i.old_position.x, i.old_position.y));
	}

	hierarchy.map_file_write("war3map.doo", writer.buffer);
}

void Doodads::create() {
	for (auto&& i : doodads) {
		i.init(i.id, get_mesh(i.id, i.variation));
	}

	for (auto&& i : special_doodads) {
		i.mesh = get_mesh(i.id, i.variation);
		i.skeleton = SkeletalModelInstance(i.mesh->model);
		const std::string pathing_texture_path = doodads_slk.data("pathtex", i.id);
		if (hierarchy.file_exists(pathing_texture_path)) {
			i.pathing = resource_manager.load<PathingTexture>(pathing_texture_path);
			i.position += glm::vec3(glm::vec2(i.pathing->width / 8.f, i.pathing->height / 8.f), 0.f);
		}

		i.position.z = map->terrain.interpolated_height(i.position.x, i.position.y, true);

		float rotation = doodads_slk.data<int>("fixedrot", i.id) / 360.f * 2.f * glm::pi<float>();
		i.matrix = glm::translate(i.matrix, i.position);
		i.matrix = glm::scale(i.matrix, { 1.f / 128.f, 1.f / 128.f, 1.f / 128.f });
		i.matrix = glm::rotate(i.matrix, rotation, glm::vec3(0, 0, 1));
	}

	// Blit doodad pathing
	for (const auto& i : doodads) {
		if (!i.pathing) {
			continue;
		}

		map->pathing_map.blit_pathing_texture(i.position, glm::degrees(i.angle) + 90, i.pathing);
	}
	map->pathing_map.upload_dynamic_pathing();

	// Update terrain exists
	update_special_doodad_pathing(QRect(0, 0, map->terrain.width, map->terrain.height));
}

// Will assign a creation number
Doodad& Doodads::add_doodad(std::string id, int variation, glm::vec3 position) {
	Doodad doodad;
	doodad.id = id;
	doodad.skin_id = id;
	doodad.variation = variation;
	doodad.mesh = get_mesh(id, variation);
	doodad.position = position;
	doodad.scale = { 1, 1, 1 };
	doodad.angle = 0;
	doodad.creation_number = ++Doodad::auto_increment;
	doodad.skeleton = SkeletalModelInstance(doodad.mesh->model);

	const bool is_doodad = doodads_slk.row_headers.contains(id);
	const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;
	std::string pathing_texture_path = slk.data("pathtex", id);
	if (hierarchy.file_exists(pathing_texture_path)) {
		doodad.pathing = resource_manager.load<PathingTexture>(pathing_texture_path);
	}

	doodad.update();

	doodads.push_back(doodad);
	return doodads.back();
}

// You will have to manually set a creation number and valid skin ID
Doodad& Doodads::add_doodad(Doodad doodad) {
	doodads.push_back(doodad);
	return doodads.back();
}

void Doodads::remove_doodad(Doodad* doodad) {
	auto iterator = doodads.begin() + std::distance(doodads.data(), doodad);
	doodads.erase(iterator);
}

std::vector<Doodad*> Doodads::query_area(const QRectF& area) {
	std::vector<Doodad*> result;

	for (auto&& i : doodads) {
		if (area.contains(i.position.x, i.position.y)) {
			result.push_back(&i);
		}
	}
	return result;
}

void Doodads::remove_doodads(const std::unordered_set<Doodad*>& list) {
	std::erase_if(doodads, [&](Doodad& doodad) {
		return list.contains(&doodad); 
	});
}

void Doodads::update_doodad_pathing(const std::vector<Doodad>& target_doodads) {
	QRectF update_pathing_area;
	for (const auto& i : target_doodads) {
		if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
			update_pathing_area = { i.position.x, i.position.y, 1.f, 1.f };
		}
		update_pathing_area |= { i.position.x, i.position.y, 1.f, 1.f };
	}

	update_doodad_pathing(update_pathing_area);
}

void Doodads::update_doodad_pathing(const std::unordered_set<Doodad*>& target_doodads) {
	QRectF update_pathing_area;
	for (const auto& i : target_doodads) {
		if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
			update_pathing_area = { i->position.x, i->position.y, 1.f, 1.f };
		}
		update_pathing_area |= { i->position.x, i->position.y, 1.f, 1.f };
	}

	update_doodad_pathing(update_pathing_area);
}

void Doodads::update_doodad_pathing(const QRectF& area) {
	QRectF new_area = area.adjusted(-6, -6, 6, 6);
	map->pathing_map.dynamic_clear_area(new_area.toRect());

	new_area.adjust(-6, -6, 6, 6);

	const auto doodads_to_blit = query_area(new_area);
	for (const auto& i : doodads_to_blit) {
		if (!i->pathing) {
			continue;
		}
		map->pathing_map.blit_pathing_texture(i->position, glm::degrees(i->angle) + 90, i->pathing);
	}
	map->pathing_map.upload_dynamic_pathing();
}

void Doodads::update_special_doodad_pathing(const QRectF& area) {
	QRectF new_area = area.adjusted(-6.f, -6.f, 6.f, 6.f);
	new_area = new_area.intersected({ 0, 0, static_cast<float>(map->terrain.width), static_cast<float>(map->terrain.height) });

	for (int i = new_area.left(); i < new_area.right(); i++) {
		for (int j = new_area.top(); j < new_area.bottom(); j++) {
			map->terrain.corners[i][j].special_doodad = false;
		}
	}

	new_area = area.adjusted(-6, -6, 6, 6);

	for (const auto& i : special_doodads) {
		if (!new_area.contains(i.position.x, i.position.y)) {
			continue;
		}

		if (!i.pathing) {
			continue;
		}

		for (int j = 0; j < i.pathing->width / 4; j++) {
			for (int k = 0; k < i.pathing->height / 4; k++) {
				const int x = i.position.x - i.pathing->width / 8.f + j;
				const int y = i.position.y - i.pathing->height / 8.f + k;

				if (x < 0 || y < 0 || x >= map->terrain.width || y >= map->terrain.height) {
					continue;
				}

				map->terrain.corners[x][y].special_doodad = true;
			}
		}
	}

	map->terrain.update_ground_exists(area.toRect());
}

void Doodads::process_doodad_field_change(const std::string& id, const std::string& field) {
	context->makeCurrent();

	if (field == "file" || field == "numvar") {
		// id_to_mesh requires a variation too so we will just have to check a bunch of them
		// ToDo just use the numvar field from the SLKs
		for (int i = 0; i < 20; i++) {
			id_to_mesh.erase(id + std::to_string(i));
		}
		for (auto& i : doodads) {
			if (i.id == id) {
				i.mesh = get_mesh(id, i.variation);
				i.skeleton = SkeletalModelInstance(i.mesh->model);
				i.update();
			}
		}
	}

	if (field == "maxroll" || field == "maxpitch" || field.starts_with("vert")) {
		for (auto& i : doodads) {
			if (i.id == id) {
				i.update();
			}
		}
	}

	if (field == "pathtex") {
		const std::string pathing_texture_path = doodads_slk.data("pathtex", id);

		if (hierarchy.file_exists(pathing_texture_path)) {
			auto new_pathing_texture = resource_manager.load<PathingTexture>(pathing_texture_path);
			for (auto& i : doodads) {
				if (i.id == id) {
					i.pathing = new_pathing_texture;
				}
			}
		} else {
			return;
		}
	}
}

void Doodads::process_destructible_field_change(const std::string& id, const std::string& field) {
	context->makeCurrent();

	if (field == "file" || field == "numvar") {
		// id_to_mesh requires a variation too so we will just have to check a bunch of them
		// ToDo just use the numvar field from the SLKs
		for (int i = 0; i < 20; i++) {
			id_to_mesh.erase(id + std::to_string(i));
		}
		for (auto& i : doodads) {
			if (i.id == id) {
				i.mesh = get_mesh(id, i.variation);
				i.skeleton = SkeletalModelInstance(i.mesh->model);
				i.update();
				i.skeleton.update(0.016f);
			}
		}
	}

	if (field == "maxroll" || field == "maxpitch" || field.starts_with("color")) {
		for (auto& i : doodads) {
			if (i.id == id) {
				i.update();
			}
		}
	}

	if (field == "pathtex") {
		const std::string pathing_texture_path = destructibles_slk.data("pathtex", id);

		if (hierarchy.file_exists(pathing_texture_path)) {
			auto new_pathing_texture = resource_manager.load<PathingTexture>(pathing_texture_path);
			for (auto& i : doodads) {
				if (i.id == id) {
					i.pathing = new_pathing_texture;
				}
			}
		} else {
			return;
		}
	}
}

std::shared_ptr<SkinnedMesh> Doodads::get_mesh(std::string id, int variation) {
	std::string full_id = id + std::to_string(variation);
	if (id_to_mesh.contains(full_id)) {
		return id_to_mesh[full_id];
	}

	fs::path mesh_path;
	std::string variations;
	std::string replaceable_id;
	fs::path texture_name;

	if (doodads_slk.row_headers.contains(id)) {
		// Is doodad
		mesh_path = doodads_slk.data("file", id);
		variations = doodads_slk.data("numvar", id);
	} else {
		mesh_path = destructibles_slk.data("file", id);
		variations = destructibles_slk.data("numvar", id);

		replaceable_id = destructibles_slk.data("texid", id);
		texture_name = destructibles_slk.data("texfile", id);
		texture_name.replace_extension("");
	}

	const std::string stem = mesh_path.stem().string();
	mesh_path.replace_filename(stem + (variations == "1" ? "" : std::to_string(variation)));
	mesh_path.replace_extension(".mdx");

	// Use base model when variation doesn't exist
	if (!hierarchy.file_exists(mesh_path)) {
		mesh_path.remove_filename() /= stem + ".mdx";
	}

	mesh_path = fs::path(string_replaced(mesh_path.string(), "\\", "/"));

	// Mesh doesn't exist at all
	if (!hierarchy.file_exists(mesh_path)) {
		std::println("Invalid model file for {} with file path: {}", id, mesh_path.string());
		id_to_mesh.emplace(full_id, resource_manager.load<SkinnedMesh>("Objects/Invalidmodel/Invalidmodel.mdx", "", std::nullopt));
		return id_to_mesh[full_id];
	}

	if (is_number(replaceable_id) && texture_name != "_") {
		id_to_mesh.emplace(full_id, resource_manager.load<SkinnedMesh>(mesh_path, texture_name.string(), std::make_optional(std::make_pair(std::stoi(replaceable_id), texture_name.replace_extension("").string()))));
	} else {
		id_to_mesh.emplace(full_id, resource_manager.load<SkinnedMesh>(mesh_path, "", std::nullopt));
	}

	return id_to_mesh[full_id];
}

void DoodadAddAction::undo() {
	map->doodads.doodads.resize(map->doodads.doodads.size() - doodads.size());
	map->doodads.update_doodad_pathing(doodads);
}

void DoodadAddAction::redo() {
	map->doodads.doodads.insert(map->doodads.doodads.end(), doodads.begin(), doodads.end());
	map->doodads.update_doodad_pathing(doodads);
}

void DoodadDeleteAction::undo() {
	if (map->brush) {
		map->brush->clear_selection();
	}

	map->doodads.doodads.insert(map->doodads.doodads.end(), doodads.begin(), doodads.end());
	map->doodads.update_doodad_pathing(doodads);
}

void DoodadDeleteAction::redo() {
	if (map->brush) {
		map->brush->clear_selection();
	}

	map->doodads.doodads.resize(map->doodads.doodads.size() - doodads.size());
	map->doodads.update_doodad_pathing(doodads);
}

void DoodadStateAction::undo() {
	QRectF update_pathing_area;
	for (auto& i : old_doodads) {
		for (auto& j : map->doodads.doodads) {
			if (i.creation_number == j.creation_number) {
				if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
					update_pathing_area = { j.position.x, j.position.y, 1.f, 1.f };
				}
				update_pathing_area |= { j.position.x, j.position.y, 1.f, 1.f };
				update_pathing_area |= { i.position.x, i.position.y, 1.f, 1.f };

				j = i;
			}
		}
	}
	map->doodads.update_doodad_pathing(update_pathing_area);
}

void DoodadStateAction::redo() {
	QRectF update_pathing_area;
	for (auto& i : new_doodads) {
		for (auto& j : map->doodads.doodads) {
			if (i.creation_number == j.creation_number) {
				if (update_pathing_area.width() == 0 || update_pathing_area.height() == 0) {
					update_pathing_area = { j.position.x, j.position.y, 1.f, 1.f };
				}
				update_pathing_area |= { j.position.x, j.position.y, 1.f, 1.f };
				update_pathing_area |= { i.position.x, i.position.y, 1.f, 1.f };

				j = i;
			}
		}
	}
	map->doodads.update_doodad_pathing(update_pathing_area);
}
