#include "qt_settings_bridge.h"

#include <QSettings>

std::optional<std::string> hivewe_warcraft_directory_setting() {
    QSettings settings;

    if (!settings.contains("warcraftDirectory")) {
        return std::nullopt;
    }

    return settings.value("warcraftDirectory").toString().toStdString();
}
