#include "brush_render_bridge.h"

#include "brush.h"

void upload_brush_position(
    const Brush* brush,
    int location
) {
    const glm::vec2 position = brush->get_position();
    glUniform2fv(location, 1, &position[0]);
}

bool brush_is_selection_mode(
    const Brush* brush
) {
    return brush->get_mode() == Brush::Mode::selection;
}

unsigned int brush_texture_id(
    const Brush* brush
) {
    return brush->brush_texture;
}

void clear_brush_selection(
    Brush* brush
) {
    brush->clear_selection();
}

void notify_brush_selection_changed(
    Brush* brush
) {
    brush->selection_changed();
}
