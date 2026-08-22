#include "tileset_dialogs.h"

#include <QMessageBox>
#include <QString>

void show_tileset_save_error(const std::string& path) {
    QMessageBox::critical(
        nullptr,
        "Error saving custom pathing",
        QString("Failed to save %1").arg(QString::fromStdString(path))
    );
}

void show_tileset_load_error(const std::string& path, const std::string& error) {
    QMessageBox::critical(
        nullptr,
        "Error loading terrain pathing",
        QString("Failed to load %1:\n%2")
            .arg(QString::fromStdString(path))
            .arg(QString::fromStdString(error))
    );
}
