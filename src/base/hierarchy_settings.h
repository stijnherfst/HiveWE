#pragma once

struct HierarchyRuntimeSettings {
    bool ptr;
    bool hd;
    bool teen;
};

bool hierarchy_allow_local_files_setting();
HierarchyRuntimeSettings hierarchy_runtime_settings();
