#pragma once

#include <string>
#include <string_view>

void map_show_critical_error(
    std::string_view title,
    std::string_view message
);

void map_show_message(
    std::string_view message
);

void map_show_information(
    std::string_view title,
    std::string_view message
);

void map_brush_unselect_id(
    void* brush,
    const std::string& id
);

void map_brush_render(
    void* brush
);

void map_update_region_render_buffer(
    void* regions,
    void* brush
);
