module;

#include "main_window/glwidget.h"

export module Globals;

import std;
import INI;
import SLK;

export inline ini::INI world_edit_strings;
export inline ini::INI world_edit_game_strings;
export inline ini::INI world_edit_data;

export inline slk::SLK units_slk;
export inline slk::SLK units_meta_slk;
export inline ini::INI unit_editor_data;

export inline slk::SLK items_slk;
export inline slk::SLK items_meta_slk;

export inline slk::SLK abilities_slk;
export inline slk::SLK abilities_meta_slk;

export inline slk::SLK doodads_slk;
export inline slk::SLK doodads_meta_slk;

export inline slk::SLK destructibles_slk;
export inline slk::SLK destructibles_meta_slk;

export inline slk::SLK upgrade_slk;
export inline slk::SLK upgrade_meta_slk;

export inline slk::SLK buff_slk;
export inline slk::SLK buff_meta_slk;

export inline GLWidget* context;

/// Generates a four character ID that is not in use by any of the object types
export std::string get_unique_id(bool first_uppercase) {
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<int> dist(0, 25);
again:

	std::string id = std::string("") + char((first_uppercase ? 'A' : 'a') + dist(mt)) + char('a' + dist(mt)) + char('a' + dist(mt))
					 + char('a' + dist(mt));

	if (units_slk.row_headers.contains(id) || items_slk.row_headers.contains(id) || abilities_slk.row_headers.contains(id)
		|| doodads_slk.row_headers.contains(id) || destructibles_slk.row_headers.contains(id) || upgrade_slk.row_headers.contains(id)
		|| buff_slk.row_headers.contains(id)) {
		std::print("Generated an existing ID: {} what're the odds\n", id);
		goto again;
	}

	return id;
}