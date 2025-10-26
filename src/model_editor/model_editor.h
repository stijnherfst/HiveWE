#pragma once

#include <filesystem>
namespace fs = std::filesystem;
#include <expected>

#include <QMainWindow>

#include <DockManager.h>
#include <DockAreaWidget.h>

class ModelEditor : public QMainWindow {
	Q_OBJECT

public:
	ModelEditor(QWidget* parent = nullptr);

	std::expected<void, std::string> open_model(const fs::path& path, bool local_file) const;

private:
	ads::CDockManager* dock_manager;
	ads::CDockAreaWidget* dock_area = nullptr;
};