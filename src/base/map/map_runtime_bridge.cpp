#include "map_runtime_bridge.h"

#include <string>

#include <QMessageBox>
#include <QString>

#include "brush/brush.h"
#include "brush/region_brush.h"

namespace {
QString to_qstring(
    const std::string_view value
) {
    return QString::fromStdString(
        std::string(value)
    );
}
}

void map_show_critical_error(
    const std::string_view title,
    const std::string_view message
) {
    QMessageBox::critical(
        nullptr,
        to_qstring(title),
        to_qstring(message)
    );
}

void map_show_message(
    const std::string_view message
) {
    QMessageBox msgbox;
    msgbox.setText(to_qstring(message));
    msgbox.exec();
}

void map_show_information(
    const std::string_view title,
    const std::string_view message
) {
    QMessageBox::information(
        nullptr,
        to_qstring(title),
        to_qstring(message),
        QMessageBox::StandardButton::Ok
    );
}

void map_brush_unselect_id(
    void* brush,
    const std::string& id
) {
    if (!brush) {
        return;
    }

    static_cast<Brush*>(brush)
        ->unselect_id(id);
}

void map_brush_render(
    void* brush
) {
    if (!brush) {
        return;
    }

    static_cast<Brush*>(brush)
        ->render();
}

void map_update_region_render_buffer(
    void* regions,
    void* brush
) {
    auto& typed_regions =
        *static_cast<Regions*>(regions);

    auto* region_brush =
        dynamic_cast<RegionBrush*>(
            static_cast<Brush*>(brush)
        );

    typed_regions.update_render_buffer(
        region_brush
            ? &region_brush->selections
            : nullptr
    );
}
