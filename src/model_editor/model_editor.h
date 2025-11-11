#pragma once

#include <filesystem>
namespace fs = std::filesystem;
#include <expected>

#include "global_search.h"

#include <QMainWindow>
#include <QKeyEvent>

#include <DockManager.h>
#include <DockAreaWidget.h>

class ModelEditor : public QMainWindow {
	Q_OBJECT

public:
	explicit ModelEditor(QWidget* parent = nullptr);

	[[nodiscard]] std::expected<void, std::string> open_model(const fs::path& path, bool local_file) const;


	void keyPressEvent(QKeyEvent* event) override {
		if (event->key() == Qt::Key_Shift && !event->isAutoRepeat()) {
			if (double_shift_timer.isValid() && double_shift_timer.elapsed() < 400) {

				GlobalSearchWidget search_widget = new GlobalSearchWidget(this);
				double_shift_timer.invalidate();
			} else {
				double_shift_timer.start();
			}
		}
		QMainWindow::keyPressEvent(event);
	}

private:
	ads::CDockManager* dock_manager;
	ads::CDockAreaWidget* dock_area = nullptr;

	QElapsedTimer double_shift_timer;
};