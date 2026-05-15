#pragma once

#include <glad/glad.h>
#include <bitset>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <QElapsedTimer>
#define QT_NO_OPENGL
#include <QObject>
#include <QOpenGLWidget>

import EditableMesh;
import SkeletalModelInstance;
import MDX;
import Shader;
import <glm/glm.hpp>;

enum class ModelCategory {
	Abilities,
	Buildings,
	Doodads,
	Environment,
	Objects,
	SharedModels,
	Units,
	Map,
	Count
};

struct ModelEntry {
	std::filesystem::path path;
	ModelCategory category;
};

class ModelGridGLWidget : public QOpenGLWidget {
	Q_OBJECT

  public:
	ModelGridGLWidget() = delete;
	explicit ModelGridGLWidget(const std::vector<ModelEntry>& entries, QWidget* parent = nullptr);
	~ModelGridGLWidget() override;

	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

	void mousePressEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

	int cell_pixel_size() const { return cell_size; }

  public slots:
	void set_scroll_offset(int y);
	void set_search(const QString& query);
	void set_categories(std::bitset<static_cast<size_t>(ModelCategory::Count)> mask);

  signals:
	void clicked(const std::filesystem::path& path);
	void scroll_changed(int offset);
	void content_height_changed(int total_px);

  private:
	struct PreviewCell {
		std::filesystem::path path;
		ModelCategory category;
		std::shared_ptr<mdx::MDX> mdx;
		std::shared_ptr<EditableMesh> mesh;
		SkeletalModelInstance skeleton;
		float fit_distance = 0.f;
		glm::vec3 fit_position{ 0.f, 0.f, 0.f };
		bool loaded = false;
		bool load_failed = false;
	};

	struct LayoutRow {
		enum class Kind { Header, Cells };
		Kind kind;
		ModelCategory category;
		int visible_offset = 0;
		int visible_count = 0;
		int y_top = 0;
		int height = 0;
	};

	std::vector<PreviewCell> all_cells;
	std::vector<int> visible_indices;
	std::vector<LayoutRow> layout;
	std::shared_ptr<Shader> shader;
	GLuint vao = 0;

	int cell_size = 128;
	int header_height = 40;
	int columns = 1;
	int scroll_offset_y = 0;

	std::string search_query;
	std::bitset<static_cast<size_t>(ModelCategory::Count)> category_mask;

	QElapsedTimer elapsed_timer;
	double delta = 0.0;

	void load_cell(PreviewCell& cell);
	void rebuild_layout();
	int content_height_px() const;
	int max_scroll_offset() const;
	void emit_layout_change();
};
