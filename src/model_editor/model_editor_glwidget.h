#pragma once

#include <glad/glad.h>
#include <memory>
#include <vector>
#include <QElapsedTimer>
#define QT_NO_OPENGL
#include <QObject>
#include <QOpenGLWidget>
#include <QFileSystemWatcher>
#include <string>
#include "qt_imgui/qt_imGui.h"
#include <model_editor/model_editor_camera.h>

import EditableMesh;
import Skeleton;
import MDX;
import Shader;

class ModelEditorGLWidget: public QOpenGLWidget {
	Q_OBJECT

  public:
	QElapsedTimer elapsed_timer;
	GLuint vao;

	double delta = 0.0;

	ModelEditorGLWidget() = delete;
	explicit ModelEditorGLWidget(QWidget* parent, const std::shared_ptr<mdx::MDX>& model, std::vector<mdx::ValidationMessage> messages = {});
	~ModelEditorGLWidget() = default;

	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

	std::shared_ptr<mdx::MDX> mdx;
	std::vector<mdx::ValidationMessage> messages;
	std::shared_ptr<EditableMesh> mesh;
	Skeleton skeleton;
	std::shared_ptr<Shader> shader_sd;
	std::shared_ptr<Shader> shader_hd;

	/// For extent and grid drawing (both reuse line_vao/line_vbo)
	std::shared_ptr<Shader> line_shader;
	std::shared_ptr<Shader> grid_shader;
	GLuint line_vao = 0;
	GLuint line_vbo = 0;
	bool draw_extents_box = false;
	bool draw_extents_sphere = false;
	bool draw_grid = true;

	/// Validation severity filters
	bool filter_error = true;
	bool filter_severe = true;
	bool filter_warning = true;
	bool filter_unused = true;

	/// Edit MDL hot-reload session: temp .mdl path is watched, edits on disk reload the model
	QFileSystemWatcher* mdl_watcher = nullptr;
	std::string hot_reload_path;
	bool reload_pending = false;

	/// Index of the texture shown enlarged in the Textures tab, or -1 for none
	int selected_texture = -1;

	bool animation_paused = false;

	/// Set in initializeGL when no saved imgui layout exists, consumed once in paintGL to build the
	/// default floating dock group.
	bool build_default_layout = false;

	int64_t optimization_file_size_reduction = 0;
	float optimization_file_size_reduction_percent = 0.f;

	QtImGui::RenderRef ref = nullptr;
	ModelEditorCamera camera;

	void recenter_camera();
	void render_extents();
	void render_grid();
	void reload_from_mdl();

	/// Builds and renders the whole ImGui interface for one frame.
	void render_imgui();
};
