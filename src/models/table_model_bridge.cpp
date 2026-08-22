#include "table_model_bridge.h"

#include <utility>

#include "models/table_model.h"

namespace {
TableModel* global_table_model(GlobalTableModelKind kind) {
    switch (kind) {
        case GlobalTableModelKind::units:
            return static_cast<TableModel*>(units_table);
        case GlobalTableModelKind::items:
            return static_cast<TableModel*>(items_table);
        case GlobalTableModelKind::abilities:
            return static_cast<TableModel*>(abilities_table);
        case GlobalTableModelKind::doodads:
            return static_cast<TableModel*>(doodads_table);
        case GlobalTableModelKind::destructibles:
            return static_cast<TableModel*>(destructibles_table);
        case GlobalTableModelKind::upgrades:
            return static_cast<TableModel*>(upgrade_table);
        case GlobalTableModelKind::buffs:
            return static_cast<TableModel*>(buff_table);
    }

    return nullptr;
}

void set_global_table_model(
    GlobalTableModelKind kind,
    TableModel* model
) {
    switch (kind) {
        case GlobalTableModelKind::units:
            units_table = model;
            break;
        case GlobalTableModelKind::items:
            items_table = model;
            break;
        case GlobalTableModelKind::abilities:
            abilities_table = model;
            break;
        case GlobalTableModelKind::doodads:
            doodads_table = model;
            break;
        case GlobalTableModelKind::destructibles:
            destructibles_table = model;
            break;
        case GlobalTableModelKind::upgrades:
            upgrade_table = model;
            break;
        case GlobalTableModelKind::buffs:
            buff_table = model;
            break;
    }
}
}

void create_global_table_model(
    GlobalTableModelKind kind,
    void* slk_data,
    void* meta_slk_data,
    void* trigger_strings_data
) {
    auto* model = new TableModel(
        static_cast<slk::SLK*>(slk_data),
        static_cast<slk::SLK*>(meta_slk_data),
        static_cast<TriggerStrings*>(trigger_strings_data)
    );

    set_global_table_model(kind, model);
}

void connect_global_table_model_data_changed(
    GlobalTableModelKind kind,
    std::function<void(int row, int column)> callback
) {
    TableModel* model = global_table_model(kind);

    QObject::connect(
        model,
        &QAbstractItemModel::dataChanged,
        model,
        [callback = std::move(callback)](
            const QModelIndex& top_left
        ) {
            callback(
                top_left.row(),
                top_left.column()
            );
        }
    );
}

void connect_global_table_model_rows_about_to_be_removed(
    GlobalTableModelKind kind,
    std::function<void(int first, int last)> callback
) {
    TableModel* model = global_table_model(kind);

    QObject::connect(
        model,
        &QAbstractItemModel::rowsAboutToBeRemoved,
        model,
        [callback = std::move(callback)](
            const QModelIndex&,
            int first,
            int last
        ) {
            callback(first, last);
        }
    );
}

void copy_table_model_row(
    QAbstractItemModel* model,
    std::string_view row_header,
    std::string_view new_row_header
) {
    static_cast<TableModel*>(model)->copyRow(
        row_header,
        new_row_header
    );
}
