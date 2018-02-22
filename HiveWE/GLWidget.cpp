#include "stdafx.h"

void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, void *userParam) {
	// Ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204 || id == 8) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source) {
		case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
		case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} 
	std::cout << std::endl;

	switch (type) {
		case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
		case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} 
	std::cout << std::endl;

	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
		case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} 
	std::cout << std::endl;
	//std::cout << id << "\n";
}

GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent) {
	connect(&timer, &QTimer::timeout, this, &GLWidget::updateScene);
	timer.start(15);

	setMouseTracking(true);
	setFocus();
}

GLWidget::~GLWidget() {
}

void GLWidget::initializeGL() {
	gl = new QOpenGLExtraFunctions;
	gl->initializeOpenGLFunctions();

	gl->glEnable(GL_DEBUG_OUTPUT);
	gl->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	gl->glDebugMessageCallback((GLDEBUGPROC)glDebugOutput, nullptr);
	gl->glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	gl->glEnable(GL_DEPTH_TEST);
	gl->glDepthFunc(GL_LEQUAL);
	gl->glEnable(GL_BLEND);
	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gl->glClearColor(0, 0, 0, 1);
	//gl->glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	gl->glGenVertexArrays(1, &vao);
	gl->glBindVertexArray(vao);

	shapes.init();

	map.load(L"Data/Test.w3x");
}

void GLWidget::resizeGL(int w, int h) {
	gl->glViewport(0, 0, w, h);

	double delta = elapsedTimer.nsecsElapsed() / 1'000'000'000.0;
	camera.aspect_ratio = (double) w / h;
	camera.update(delta);
}

void GLWidget::updateScene() {
	double delta = elapsedTimer.nsecsElapsed() / 1'000'000'000.0;
	elapsedTimer.start();

	camera.update(delta);

	map.terrain.current_texture += std::max(0.0, map.terrain.animation_rate * delta);
	if (map.terrain.current_texture >= map.terrain.water_textures_nr) {
		map.terrain.current_texture = 0;
	}

	update();
}

void GLWidget::paintGL() {
	gl->glClearColor(0, 0, 0, 1);
	gl->glClear(GL_DEPTH_BUFFER_BIT);

	gl->glBindVertexArray(vao);
	map.render();

	gl->glBindVertexArray(0);

	// QPainter does some wonky stuff with OpenGL state.
	//QPainter p(this);
	//p.setFont(QFont("Arial", 10, 100, false));
	//p.drawText(10, 20, QString::fromStdString("Terrain Drawing: " + std::to_string(map.terrain_time)));
	//	p.drawText(20, 35, QString::fromStdString("Terrain Tiles: " + std::to_string(map.terrain_tiles_time)));
	//	p.drawText(20, 50, QString::fromStdString("Terrain Cliffs: " + std::to_string(map.terrain_cliff_time)));
	//	p.drawText(20, 65, QString::fromStdString("Terrain Water: " + std::to_string(map.terrain_water_time)));
	//p.drawText(10, 80, QString::fromStdString("Doodad Queue: " + std::to_string(map.queue_time)));
	//p.drawText(10, 95, QString::fromStdString("Doodad Drawing: " + std::to_string(map.doodad_time)));

	gl->glBindVertexArray(vao);
}

void GLWidget::keyPressEvent(QKeyEvent *e) {
	input_handler.keys_pressed.emplace(e->key());
	switch (e->key()) {
		case Qt::Key_Escape:
			exit(0);
			break;
		case Qt::Key_Alt:
			//input_handler.drag_start = input_handler.mouse_world;
		case Qt::Key_Equal:
			map.brush.set_size(map.brush.size + 1);
			break;
		case Qt::Key_Minus:
			map.brush.set_size(map.brush.size - 1);
			break;
	}
}

void GLWidget::keyReleaseEvent(QKeyEvent *e) {
	input_handler.keys_pressed.erase(e->key());
}

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
	input_handler.mouse_move_event(event);
	camera.mouse_move_event(event);

	makeCurrent();
	QPoint cursor_position = mapFromGlobal(QCursor::pos());
	
	float z;
	gl->glReadPixels(cursor_position.x(), height() - cursor_position.y(), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
	glm::vec3 window = glm::vec3(cursor_position.x(), height() - cursor_position.y(), z);

	input_handler.mouse_world = glm::unProject(window, camera.view, camera.projection, glm::vec4(0, 0, width(), height()));

	//if (input_handler.key_pressed(Qt::Key_Alt)) {
	//	map.brush.set_size(map.brush.size + (input_handler.mouse_world - input_handler.drag_start).x);
	//} else {
		map.brush.set_position(input_handler.mouse_world);

		if (event->buttons() == Qt::LeftButton) {
			map.brush.apply(map.pathing_map);
		}
	//}
}

void GLWidget::wheelEvent(QWheelEvent* event) {
	camera.mouse_scroll_event(event);
}