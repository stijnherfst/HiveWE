#include "ModelEditorGLWidget.h"

#include "fmt/format.h"

//#include <glad/glad.h>

#include <QTimer>
//#include <QOpenGLFunctions_4_5_Core>
#include <QPainter>

//#include "Utilities.h"
#include "InputHandler.h"
#include <QtImgui/QtImGui.h>
#include <imgui.h>

#include "HiveWE.h"


ModelEditorGLWidget::ModelEditorGLWidget(QWidget* parent) : QOpenGLWidget(parent) {
	//QTimer::singleShot(16, this, &GLWidget::update_scene);

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
	// mesh = std::make_shared<EditableMesh>("units/human/footman/footman.mdx", "", std::nullopt);
	skeleton = SkeletalModelInstance(mesh->model);

	shader = resource_manager.load<Shader>({ "Data/Shaders/editable_mesh_hd.vs", "Data/Shaders/editable_mesh_hd.fs" });
	test_shader = resource_manager.load<Shader>({ "Data/Shaders/test.vs", "Data/Shaders/test.fs" });

	begin = std::chrono::steady_clock::now();

	QTimer::singleShot(16.67, this, &ModelEditorGLWidget::update_scene);
}

void ModelEditorGLWidget::resizeGL(const int w, const int h) {
	gl->glViewport(0, 0, w, h);

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

	//gl->glEnable(GL_DEPTH_TEST);
	//gl->glDepthMask(true);
	//gl->glEnable(GL_CULL_FACE);
	//gl->glDepthFunc(GL_LEQUAL);

	//gl->glClearColor(0.25f, 0.25f, 0.f, 1.f);
	//gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//gl->glEnable(GL_BLEND);

	gl->glBindVertexArray(vao);
	gl->glEnable(GL_DEPTH_TEST);
	gl->glDepthMask(true);
	gl->glClearColor(0.f, 0.f, 0.f, 1.f);
	gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->use();
	mesh->render(skeleton, camera.projection_view);

	gl->glEnable(GL_BLEND);

	gl->glBindVertexArray(0);

	gl->glEnable(GL_DEPTH_TEST);
	gl->glDepthFunc(GL_LEQUAL);
	gl->glEnable(GL_BLEND);
	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	QtImGui::newFrame();

	for (const auto& i : mesh->model->textures) {
		ImGui::Text(i.file_name.string().c_str());
	}
	ImGui::Text("Hello");
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