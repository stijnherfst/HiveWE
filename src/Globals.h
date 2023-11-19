#pragma once

#include "main_window/glwidget.h"

#include <ankerl/unordered_dense.h>

import WindowHandler;
import INI;
import SLK;

inline ini::INI world_edit_strings;
inline ini::INI world_edit_game_strings;
inline ini::INI world_edit_data;

inline slk::SLK units_slk;
inline slk::SLK units_meta_slk;
inline ini::INI unit_editor_data;

inline slk::SLK items_slk;
inline slk::SLK items_meta_slk;

inline slk::SLK abilities_slk;
inline slk::SLK abilities_meta_slk;

inline slk::SLK doodads_slk;
inline slk::SLK doodads_meta_slk;

inline slk::SLK destructibles_slk;
inline slk::SLK destructibles_meta_slk;

inline slk::SLK upgrade_slk;
inline slk::SLK upgrade_meta_slk;

inline slk::SLK buff_slk;
inline slk::SLK buff_meta_slk;

inline GLWidget* context;

inline WindowHandler window_handler;