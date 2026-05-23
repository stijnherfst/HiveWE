module;

#include <QObject>

module Map;

import std;
import PathingMap;

/// Resizes the entire map by expanding/shirnking it from all sides
/// Handles terrain, pathing map, shadow map and preplaced objects
/// Also, as per vanilla WE behaviour, clears the entire world undo stack
void Map::resize(const int delta_left, const int delta_right, const int delta_top, const int delta_bottom) {
	terrain.resize(delta_left, delta_right, delta_top, delta_bottom, physics);

	// update the pathing and shadow maps to the new size
	pathing_map.resize(delta_left * 4, delta_right * 4, delta_top * 4, delta_bottom * 4);
	shadow_map.resize(delta_left * 4, delta_right * 4, delta_top * 4, delta_bottom * 4);

	// update object positions
	update_object_positions(delta_left, delta_right, delta_top, delta_bottom);

	// vanilla WE behaviour: clear unit/doodad/terrain undo
	world_undo.clear_all_undo();

	// since this function does not change playable area, we have to update the map info accordingly
	info.camera_complements[0] += delta_left;
	info.camera_complements[1] += delta_right;
	info.camera_complements[2] += delta_bottom;
	info.camera_complements[3] += delta_top;

	// finally, move the camera
	camera.position.x += delta_left / 2;
	camera.position.y += delta_bottom / 2;
	camera.update(0.0);
}

/// Sets the playable area (shadowed map bounds)
/// Handles the terrain flags, camera bounds and map info
/// Also updates the pathing map and deletes units/items which are now out of bounds
void Map::set_playable_area(const int unplayable_left, const int unplayable_right, const int unplayable_top, const int unplayable_bottom) {
	// apply the shadowed camera boundaries on the edges
	terrain.set_unplayable_boundaries(unplayable_left, unplayable_right, unplayable_top, unplayable_bottom);

	// reset the pathing map - old unplayable segments should
	// no longer be unwalkable/unflyable/unbuildable
	int old_left = info.camera_complements[0];
	int old_right = info.camera_complements[1];
	int old_bottom = info.camera_complements[2];
	int old_top = info.camera_complements[3];
	reset_map_edge_pathing(old_left, old_right, old_top, old_bottom, unplayable_left, unplayable_right, unplayable_top, unplayable_bottom);

	// update map info
	info.update_map_bounds_info(
		unplayable_left,
		unplayable_right,
		unplayable_top,
		unplayable_bottom,
		terrain.width,
		terrain.height,
		terrain.offset.x,
		terrain.offset.y
	);
}

/// called when resizing the map
/// in HiveWE objeccts positions are saved relative to the bottom-left corner of the terrain
/// resizing the map may nudge all preplaced objeccts (units, doodads...)
/// calling this function will restore original positions after the resize
/// also, if will delete objects which are now out of bounds
int Map::update_object_positions(const int delta_left, const int delta_right, const int delta_top, const int delta_bottom) {
	size_t num_deleted = 0;

	// terrain is already resized here
	const int width = terrain.width - 1;
	const int height = terrain.height - 1;

	// helper to update positions and collect out-of-bounds objects for removal
	auto update_and_collect = [&](auto& container, auto&& update_func) {
		using ObjectType = std::decay_t<decltype(*container.begin())>;
		using PointerType = std::remove_reference_t<ObjectType>*;
		std::unordered_set<PointerType> to_delete;

		for (auto& obj : container) {
			// update the object position relative to the change in bottom-left corner
			obj.position.x += delta_left;
			obj.position.y += delta_bottom;

			// check if the object is outside map bounds
			if (obj.position.x < 0 || obj.position.y < 0 || obj.position.x > width || obj.position.y > height) {
				to_delete.insert(&obj);
				++num_deleted;
			} else {
				// update rendered object position
				update_func(obj);
			}
		}

		return to_delete;
	};

	// fix/remove all objects
	units.remove_units(update_and_collect(units.units, [](auto& obj) {
		obj.update();
	}));

	units.remove_items(update_and_collect(units.items, [](auto& obj) {
		obj.update();
	}));

	doodads.remove_doodads(update_and_collect(doodads.doodads, [&](auto& obj) {
		obj.update(terrain);
	}));

	doodads.remove_special_doodads(update_and_collect(doodads.special_doodads, [&](auto& obj) {
		obj.update(terrain);
	}));

	// fix and remove cameras
	std::unordered_set<GameCamera*> cameras_to_delete;
	for (GameCamera& camera : cameras.cameras) {
		// update the camera position
		camera.target_x += delta_left;
		camera.target_y += delta_bottom;

		// check if the camera is outside map bounds
		if (camera.target_x < 0 || camera.target_y < 0 || camera.target_x > width || camera.target_y > height) {
			cameras_to_delete.insert(&camera);
			++num_deleted;
		} else {
			// todo: uncomment once implemented
			// camera.update()
		}
	}
	cameras.remove_cameras(cameras_to_delete);

	// fix and remove regions
	std::unordered_set<Region*> regions_to_delete;
	for (Region& region : regions.regions) {
		// update the region position
		region.left = std::max(region.left + delta_left, 0.f);
		region.right = std::min(region.right + delta_left, float(width));
		region.bottom = std::max(region.bottom + delta_bottom, 0.f);
		region.top = std::min(region.top + delta_bottom, float(height));

		// check if the region was destroyed by the resize operation
		if (region.right <= region.left || region.top <= region.bottom) {
			regions_to_delete.insert(&region);
			++num_deleted;
		} else {
			// todo: uncomment once implemented
			// region.update()
		}
	}
	regions.remove_regions(regions_to_delete);

	return num_deleted;
}

/// Recomputes the terrain pathing on the edges of the map
void Map::reset_map_edge_pathing(
	const int old_left,
	const int old_right,
	const int old_top,
	const int old_bottom,
	const int new_left,
	const int new_right,
	const int new_top,
	const int new_bottom
) {
	int width = (terrain.width - 1) * 4;
	int height = (terrain.height - 1) * 4;

	// old unplayable segments (on the pathing map)
	int old_left_p = old_left * 4;
	int old_right_p = old_right * 4;
	int old_bottom_p = old_bottom * 4;
	int old_top_p = old_top * 4;

	// new unplayable segments (on the pathing map)
	int new_left_p = new_left * 4;
	int new_right_p = new_right * 4;
	int new_bottom_p = new_bottom * 4;
	int new_top_p = new_top * 4;

	constexpr uint8_t edge_pathing = PathingMap::unwalkable | PathingMap::unflyable | PathingMap::unbuildable;

	for (size_t i = 0; i < width; ++i) {
		for (size_t j = 0; j < height; ++j) {
			// check if outside new boundaries (in unplayable area)
			bool outside_new = (i < new_left_p || i >= width - new_right_p || j < new_bottom_p || j >= height - new_top_p);

			// check if outside old boundaries (in old unplayable area)
			bool outside_old = (i < old_left_p || i >= width - old_right_p || j < old_bottom_p || j >= height - old_top_p);

			// update the pathing map
			if (outside_new) {
				pathing_map.pathing_cells_static[j * width + i] = edge_pathing;
			} else if (outside_old) {
				pathing_map.pathing_cells_static[j * width + i] = terrain.get_terrain_pathing(i, j, true, true, true);
			}
		}
	}

	pathing_map.upload_static_pathing();
}
