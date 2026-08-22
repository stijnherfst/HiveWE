#pragma once

#include <memory>
#include <QObject>

class TerrainNotifier : public QObject {
    Q_OBJECT

  public:
    explicit TerrainNotifier(QObject* parent = nullptr)
        : QObject(parent) {}

  signals:
    void minimap_changed();
    void tileset_changed();
};

inline TerrainNotifier* terrain_notifier_from_handle(
    const std::shared_ptr<void>& handle
) noexcept {
    return static_cast<TerrainNotifier*>(handle.get());
}
