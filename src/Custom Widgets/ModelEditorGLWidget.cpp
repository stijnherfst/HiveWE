#include "ModelEditorGLWidget.h"

#include "fmt/format.h"
#include <fstream>

//#include <glad/glad.h>

#include <QTimer>
#include <QPainter>
#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QDesktopServices>

#include "InputHandler.h"
#include <QtImgui/QtImGui.h>
#include <imgui.h>

#include "HiveWE.h"

ModelEditorGLWidget::ModelEditorGLWidget(QWidget* parent) : QOpenGLWidget(parent) {
	makeCurrent();

	setMouseTracking(true);
	setFocus();
	setFocusPolicy(Qt::WheelFocus);
}

void ModelEditorGLWidget::initializeGL() {
	//gl = new QOpenGLFunctions_4_5_Core;
	//if (!gl->initializeOpenGLFunctions()) {
	//	std::cout << "Error initializing functions";
	//	throw;
	//}
	//
	//if (!gladLoadGL()) {
	//	std::cout << "Failed to initialize OpenGL context" << std::endl;
	//}
	
	QtImGui::initialize(this);

	gl->glEnable(GL_DEPTH_TEST);
	gl->glEnable(GL_CULL_FACE);
	gl->glDepthFunc(GL_LEQUAL);
	gl->glEnable(GL_BLEND);
	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gl->glClearColor(0, 0, 0, 1);
	
	gl->glGenVertexArrays(1, &vao);
	gl->glBindVertexArray(vao);

	mesh = resource_manager.load<EditableMesh>("units/human/footman/footman.mdx", "", std::nullopt);
	skeleton = SkeletalModelInstance(mesh->mdx);

	auto& extent = mesh->mdx->sequences[skeleton.sequence_index].extent;
	camera.position.z = (extent.maximum.z - extent.minimum.z) / 2.f;

	shader = resource_manager.load<Shader>({ "Data/Shaders/editable_mesh_hd.vs", "Data/Shaders/editable_mesh_hd.fs" });

	begin = std::chrono::steady_clock::now();

	QTimer::singleShot(16.67, this, &ModelEditorGLWidget::update_scene);
}

void ModelEditorGLWidget::resizeGL(const int w, const int h) {
	gl->glViewport(0, 0, w, h);
	camera.aspect_ratio = double(w) / h;
	camera.update(delta);
	delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
}

void ModelEditorGLWidget::update_scene() {
	delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	elapsed_timer.start();

	skeleton.updateLocation(glm::vec3(0.f), 0.f, glm::vec3(1.f));
	skeleton.update(delta);

	camera.update(16.666);

	update();

	QTimer::singleShot(16.67 - std::clamp(delta, 0.001, 16.60), this, &ModelEditorGLWidget::update_scene);
}

void ModelEditorGLWidget::paintGL() {
	makeCurrent();

	gl->glBindVertexArray(vao);
	gl->glEnable(GL_DEPTH_TEST);
	gl->glDepthMask(true);
	gl->glClearColor(0.3f, 0.3f, 0.3f, 1.f);
	gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->use();
	mesh->render(skeleton, camera.projection_view, camera.direction);

	gl->glEnable(GL_BLEND);

	gl->glBindVertexArray(0);

	gl->glEnable(GL_DEPTH_TEST);
	gl->glDepthFunc(GL_LEQUAL);
	gl->glEnable(GL_BLEND);
	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// ImGui
	QtImGui::newFrame();

	ImGui::Begin("General");

	if (ImGui::Button("Edit MDL")) {

		auto mdl = mesh->mdx->to_mdl();

		auto path = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + QString::fromStdString(mesh->mdx->name) + ".mdl";

		std::ofstream file(path.toStdString());
		file.write(mdl.data(), mdl.size());
		file.close();

		QDesktopServices::openUrl(QUrl(path, QUrl::TolerantMode));
	}

	if (ImGui::Button("Open Model")) {
		QSettings settings;

		QString file_name = QFileDialog::getOpenFileName(this, "Open Model File",
														 settings.value("openDirectory", QDir::current().path()).toString(),
														 "MDX (*.mdx *.MDX)");

		if (file_name == "") {
			return;
		}

		mesh = resource_manager.load<EditableMesh>(file_name.toStdString(), "", std::nullopt);
		skeleton = SkeletalModelInstance(mesh->mdx);
	}

	ImGui::Text(fmt::format("name: {}", mesh->mdx->name).c_str());

	size_t vertices = 0;
	size_t triangles = 0;
	for (const auto& i : mesh->mdx->geosets) {
		vertices += i.vertices.size();
		triangles += i.faces.size() / 3;
	}

	ImGui::Text(fmt::format("Vertices: {}", vertices).c_str());
	ImGui::Text(fmt::format("Triangles: {}", triangles).c_str());

	ImGui::End();

	ImGui::Begin("Animation");

	ImGui::Text("Animation");
	ImGui::SameLine();
	if (ImGui::BeginCombo("##combo", mesh->mdx->sequences[skeleton.sequence_index].name.c_str())) {
		for (size_t i = 0; i < mesh->mdx->sequences.size(); i++) {
			if (ImGui::Selectable(mesh->mdx->sequences[i].name.c_str(), i == skeleton.sequence_index)) {
				skeleton.set_sequence(i);
			}
			if (i == skeleton.sequence_index) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	//std::string name;
	//uint32_t start_frame;
	//uint32_t end_frame;
	//float movespeed;
	//uint32_t flags;
	//float rarity;
	//uint32_t sync_point;
	//Extent extent;

	//enum Flags {
	//	looping,
	//	non_looping
	//};

	ImGui::Text(fmt::format("Start frame: {}", mesh->mdx->sequences[skeleton.sequence_index].start_frame).c_str());
	ImGui::Text(fmt::format("End frame: {}", mesh->mdx->sequences[skeleton.sequence_index].end_frame).c_str());
	ImGui::Text(fmt::format("Current frame: {}", skeleton.current_frame).c_str());
	ImGui::Text(fmt::format("Looping: {}", mesh->mdx->sequences[skeleton.sequence_index].flags == mdx::Sequence::Flags::looping).c_str());

	ImGui::End();

	//for (const auto& i : mesh->model->textures) {
	//	ImGui::Text(i.file_name.string().c_str());
	//}
	

	// more widgets...

	ImGui::Render();
	QtImGui::render();

	//QPainter p(this);
	//p.setPen(QColor(Qt::GlobalColor::white));
	//p.setFont(QFont("Arial", 10, 100, false));

	//// Rendering time
	//p.drawText(10, 20, "Test text drawing");
}

void ModelEditorGLWidget::keyPressEvent(QKeyEvent* event) {
	my_input_handler.keys_pressed.emplace(event->key());
}

void ModelEditorGLWidget::keyReleaseEvent(QKeyEvent* event) {
	my_input_handler.keys_pressed.erase(event->key());
}

void ModelEditorGLWidget::mouseMoveEvent(QMouseEvent* event) {
	my_input_handler.mouse_move_event(event);
	camera.mouse_move_event(event, my_input_handler);
}

void ModelEditorGLWidget::mousePressEvent(QMouseEvent* event) {
	makeCurrent();
	
	camera.mouse_press_event(event);
}

void ModelEditorGLWidget::mouseReleaseEvent(QMouseEvent* event) {
	camera.mouse_release_event(event);
}

void ModelEditorGLWidget::wheelEvent(QWheelEvent* event) {
	camera.mouse_scroll_event(event);
}