export module SlkConversions;

import std;
import Map;
import MapGlobal;
import SLK;
import Globals;

/// Returns the new destructible ID
export std::string convert_doodad_to_destructible(const std::string_view doodad_id, const std::string_view base_destructible) {
	const auto new_id = map->get_unique_id(true);
	destructibles_slk.copy_row(base_destructible, new_id, false);

	const auto mapping = std::unordered_map<std::string, std::string>{
		{"fixedrot", "fixedrot"},
		{"maxroll", "maxroll"},
		{"maxpitch", "maxpitch"},
		{"showinmm", "showinmm"},
		{"usemmcolor", "usemmcolor"},
		{"mmred", "mmred"},
		{"mmgreen", "mmgreen"},
		{"mmblue", "mmblue"},
		{"file", "file"},
		{"selsize", "selsize"},
		{"showinfog", "fogvis"},
		{"vertr1", "colorr"}, // We grab the colors from variation 1
		{"vertg1", "colorg"},
		{"vertb1", "colorb"},
		{"numvar", "numvar"},
		{"canscalerandscale", "canscalerandscale"},
		{"tilesetspecific", "tilesetspecific"},
		{"maxscale", "maxscale"},
		{"minscale", "minscale"},
		{"userlist", "userlist"},
		{"oncliffs", "oncliffs"},
		{"onwater", "onwater"},
		{"tilesets", "tilesets"},
		{"useclickhelper", "useclickhelper"},
		{"pathtex", "pathtex"},
		{"walkable", "walkable"},
		{"soundloop", "loopsound"},
		{"name", "name"},
	};

	for (const auto& [header_from, header_to] : mapping) {
		const auto data = doodads_slk.data(header_from, doodad_id);
		destructibles_slk.set_shadow_data(header_to, new_id, data);
	}
	return new_id;
}
