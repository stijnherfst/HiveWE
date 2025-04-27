module;

#include "main_window/glwidget.h"

export module Globals;

import WindowHandler;
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

export inline WindowHandler window_handler;