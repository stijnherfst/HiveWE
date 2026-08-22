#include "terrain_notifier.h"
#include "terrain_notifier_bridge.h"

std::shared_ptr<void> create_terrain_notifier() {
    return std::make_shared<TerrainNotifier>();
}

void emit_terrain_minimap_changed(
    const std::shared_ptr<void>& notifier
) {
    emit static_cast<TerrainNotifier*>(
        notifier.get()
    )->minimap_changed();
}

void emit_terrain_tileset_changed(
    const std::shared_ptr<void>& notifier
) {
    emit static_cast<TerrainNotifier*>(
        notifier.get()
    )->tileset_changed();
}
