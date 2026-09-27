#pragma once

#include <memory>
#include <vector>
#include <string>
#include <QElapsedTimer>
#include <QFileSystemWatcher>
#include <QImage>
#include <QMouseEvent>
#include <QVulkanInstance>
#include <QWindow>
#include <imgui.h>
#include <volk.h>
#include <VkBootstrap.h>
#include "qt_imgui/qt_imGui.h"

// Last, as these import modules and textual standard library includes can't follow that
#include "render/vulkan_viewport.h"
#include "render/vulkan_imgui.h"
#include <model_editor/model_editor_camera.h>

import VkEditableMesh;
import VkTexture;
import Skeleton;
import MDX;

/// The model editor's 3D view with its ImGui panels, rendered with Vulkan.
/// No Q_OBJECT: this header imports modules, which must stay out of moc's combined translation unit.
class ModelEditorViewport : public VulkanViewport {
  public:
	QElapsedTimer elapsed_timer;

	double delta = 0.0;

	ModelEditorViewport() = delete;
	explicit ModelEditorViewport(const std::shared_ptr<mdx::MDX>& model, std::vector<mdx::ValidationMessage> messages = {});
	~ModelEditorViewport() override;

	/// A widget hosting a model editor viewport, or explaining why none can be shown
	static QWidget* create_widget(const std::shared_ptr<mdx::MDX>& model, std::vector<mdx::ValidationMessage> messages = {});

	std::shared_ptr<mdx::MDX> mdx;
	std::vector<mdx::ValidationMessage> messages;
	std::shared_ptr<VulkanEditableMesh> mesh;
	Skeleton skeleton;

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

	/// When false the ImGui panels are still built but not drawn, leaving the 3D view unobstructed
	bool show_ui = true;

	/// Set when no saved imgui layout exists, consumed once in render_imgui to build the
	/// default floating dock group.
	bool build_default_layout = false;

	int64_t optimization_file_size_reduction = 0;
	float optimization_file_size_reduction_percent = 0.f;

	QtImGui::RenderRef ref = nullptr;
	ModelEditorCamera camera;

	/// The widget embedding this window, for parenting dialogs
	QWidget* container = nullptr;

	/// The mesh texture at `index` in mdx->textures, or null when there is none to show
	const VulkanTexture* texture_at(size_t index) const;

	void recenter_camera();
	void reload_from_mdl();

	/// Builds the whole ImGui interface for one frame, ending with ImGui::Render().
	void render_imgui();

  protected:
	void record(VkCommandBuffer cmd, const FrameTarget& target, FrameAllocator& allocator) override;

	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

  private:
	EditableMeshRenderer mesh_renderer;
	VulkanImGuiRenderer imgui_renderer;
	Pipeline line_pipeline;

	void render_extents(VkCommandBuffer cmd, FrameAllocator& allocator);
	void render_grid(VkCommandBuffer cmd, FrameAllocator& allocator);
	void draw_lines(VkCommandBuffer cmd, FrameAllocator& allocator, const std::vector<glm::vec3>& lines, glm::vec4 color);
};
