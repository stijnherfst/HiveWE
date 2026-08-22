#pragma once

#include <memory>

std::shared_ptr<void> create_terrain_notifier();

void emit_terrain_minimap_changed(
    const std::shared_ptr<void>& notifier
);

void emit_terrain_tileset_changed(
    const std::shared_ptr<void>& notifier
);
