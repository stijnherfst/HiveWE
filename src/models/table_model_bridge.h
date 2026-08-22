#pragma once

#include <functional>
#include <string_view>

class QAbstractItemModel;

enum class GlobalTableModelKind {
    units,
    items,
    abilities,
    doodads,
    destructibles,
    upgrades,
    buffs
};

void create_global_table_model(
    GlobalTableModelKind kind,
    void* slk_data,
    void* meta_slk_data,
    void* trigger_strings_data
);

void connect_global_table_model_data_changed(
    GlobalTableModelKind kind,
    std::function<void(int row, int column)> callback
);

void connect_global_table_model_rows_about_to_be_removed(
    GlobalTableModelKind kind,
    std::function<void(int first, int last)> callback
);

void copy_table_model_row(
    QAbstractItemModel* model,
    std::string_view row_header,
    std::string_view new_row_header
);
