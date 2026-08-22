#include "hierarchy_settings.h"

#include <QSettings>

bool hierarchy_allow_local_files_setting() {
#ifdef _WIN32
    QSettings war3reg(
        "HKEY_CURRENT_USER\\Software\\Blizzard Entertainment\\Warcraft III",
        QSettings::NativeFormat
    );
    return war3reg.value("Allow Local Files", 0).toInt() != 0;
#else
    return false;
#endif
}

HierarchyRuntimeSettings hierarchy_runtime_settings() {
    QSettings settings;

    return {
        settings.value("flavour", "Retail").toString() == "PTR",
        settings.value("hd", "False").toString() == "True",
        settings.value("teen", "False").toString() == "True"
    };
}
