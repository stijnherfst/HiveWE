#pragma once

#include <QWidget>

/// Hosts the Vulkan map viewport in the main window. The viewport is a native window, so widgets
/// shown on top of it (like the minimap) must be native too.
class MapWidget : public QWidget {
	Q_OBJECT

  public:
	explicit MapWidget(QWidget* parent = nullptr);
};
