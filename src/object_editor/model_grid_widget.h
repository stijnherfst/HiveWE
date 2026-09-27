#pragma once

#include <bitset>
#include <filesystem>
#include <vector>
#include <QString>
#include <QWidget>

enum class ModelCategory {
	Map,
	Units,
	Buildings,
	Abilities,
	Doodads,
	Environment,
	Objects,
	SharedModels,
	Count
};

struct ModelEntry {
	std::filesystem::path path;
	ModelCategory category;
};

class ModelGridViewport;

/// A scrollable grid of animated model previews grouped by category, rendered with Vulkan.
class ModelGridWidget : public QWidget {
	Q_OBJECT

  public:
	ModelGridWidget() = delete;
	/// When single_preview is true the widget shows a single model filling the whole widget,
	/// without category headers or scrolling (used as a live preview thumbnail).
	explicit ModelGridWidget(const std::vector<ModelEntry>& entries, QWidget* parent = nullptr, bool single_preview = false);

	int cell_pixel_size() const;

	/// Text drawn centered when a single-preview widget has no model to show yet.
	void set_empty_message(const QString& message);

  public slots:
	void set_scroll_offset(int y);
	void set_search(const QString& query);
	void set_categories(std::bitset<static_cast<size_t>(ModelCategory::Count)> mask);

  signals:
	void clicked(const std::filesystem::path& path);
	void double_clicked(const std::filesystem::path& path);
	void scroll_changed(int offset);
	void content_height_changed(int total_px);

  private:
	/// Owned by its window container; null when Vulkan is unavailable
	ModelGridViewport* viewport = nullptr;
};
