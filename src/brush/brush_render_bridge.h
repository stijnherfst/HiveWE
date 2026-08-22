#pragma once

class Brush;

void upload_brush_position(
    const Brush* brush,
    int location
);

[[nodiscard]] bool brush_is_selection_mode(
    const Brush* brush
);

[[nodiscard]] unsigned int brush_texture_id(
    const Brush* brush
);

void clear_brush_selection(
    Brush* brush
);

void notify_brush_selection_changed(
    Brush* brush
);
