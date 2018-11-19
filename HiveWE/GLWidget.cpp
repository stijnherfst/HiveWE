#include "stdafx.h"

void APIENTRY gl_debug_output(const GLenum source, const GLenum type, const GLuint id, const GLenum severity, const GLsizei length, const GLchar *message, void *) {
	// Skip buffer info messages, framebuffer info messages, texture usage state warning, redundant state change buffer
	if (id == 131185 || id == 131169 || id == 131204 || id == 8) {
		return;
	}

	//if (id == 131218) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source) {
		case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
		case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
		default: break;
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
		default: break;
	}
	std::cout << std::endl;

	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
		case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
		default: break;
	}
	std::cout << std::endl;
}

GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent) {
	QTimer::singleShot(16, this, &GLWidget::update_scene);

	camera = &tps_camera;

	setMouseTracking(true);
	setFocus();
}

void GLWidget::initializeGL() {
	gl = new QOpenGLFunctions_4_5_Core;
	gl->initializeOpenGLFunctions();

	gl->glEnable(GL_DEBUG_OUTPUT);
	gl->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	gl->glDebugMessageCallback(GLDEBUGPROC(gl_debug_output), nullptr);
	gl->glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	gl->glEnable(GL_DEPTH_TEST);
	gl->glDepthFunc(GL_LEQUAL);
	gl->glEnable(GL_BLEND);
	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gl->glClearColor(0, 0, 0, 1);

	gl->glGenVertexArrays(1, &vao);
	gl->glBindVertexArray(vao);

	shapes.init();

	map->load("Data/Test.w3x");
}

void GLWidget::resizeGL(const int w, const int h) {
	gl->glViewport(0, 0, w, h);

	const double delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	camera->aspect_ratio = double(w) / h;
	camera->update(delta);
}

void GLWidget::update_scene() {
	const double delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	elapsed_timer.start();

	camera->update(delta);

	map->terrain.current_texture += std::max(0.0, map->terrain.animation_rate * delta);
	if (map->terrain.current_texture >= map->terrain.water_textures_nr) {
		map->terrain.current_texture = 0;
	}

	update();
	QTimer::singleShot(std::max(0.0, 16.0 - map->total_time), this, &GLWidget::update_scene);
}

void GLWidget::paintGL() {
	gl->glClearColor(0, 0, 0, 1);
	gl->glClear(GL_DEPTH_BUFFER_BIT);

	gl->glBindVertexArray(vao);
	map->render(width(), height());

	gl->glBindVertexArray(0);

	if (map->render_debug) {
		QPainter p(this);
		p.setPen(QColor(Qt::GlobalColor::white));
		p.setFont(QFont("Arial", 10, 100, false));
		// Rendering time
		p.drawText(10, 20, QString::fromStdString("Terrain Drawing: " + std::to_string(map->terrain_time)));
			p.drawText(20, 35, QString::fromStdString("Terrain Tiles: " + std::to_string(map->terrain_tiles_time)));
			p.drawText(20, 50, QString::fromStdString("Terrain Cliffs: " + std::to_string(map->terrain_cliff_time)));
			p.drawText(20, 65, QString::fromStdString("Terrain Water: " + std::to_string(map->terrain_water_time)));
		p.drawText(10, 80, QString::fromStdString("Mouse to world coordinates: " + std::to_string(map->terrain_time)));
		p.drawText(10, 95, QString::fromStdString("Doodad Queue: " + std::to_string(map->doodad_time)));
		p.drawText(10, 110, QString::fromStdString("Unit Queue: " + std::to_string(map->unit_time)));
		p.drawText(10, 125, QString::fromStdString("Render time: " + std::to_string(map->render_time)));

		p.drawText(10, 145, QString::fromStdString("Total time: " + std::to_string(map->total_time) + " Min: " + std::to_string(map->total_time_min) + " Max: " + std::to_string(map->total_time_max)));

		// General info
		p.drawText(300, 20, QString::fromStdString("Mouse Grid Position X: " + std::to_string(input_handler.mouse_world.x) + " Y: " + std::to_string(input_handler.mouse_world.y)));
		if (map->brush) {
			p.drawText(300, 35, QString::fromStdString("Brush Grid Position X: " + std::to_string(map->brush->get_position().x) + " Y: " + std::to_string(map->brush->get_position().y)));
		}

		p.drawText(300, 50, QString::fromStdString("Camera Horizontal Angle: " + std::to_string(camera->horizontal_angle)));
		p.drawText(300, 64, QString::fromStdString("Camera Vertical Angle: " + std::to_string(camera->vertical_angle)));

		p.end();

		// Set changed state back
		gl->glEnable(GL_DEPTH_TEST);
		gl->glDepthFunc(GL_LEQUAL);
		gl->glEnable(GL_BLEND);
		gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void GLWidget::keyPressEvent(QKeyEvent* event) {
	input_handler.keys_pressed.emplace(event->key());
	switch (event->key()) {
		case Qt::Key_Escape:
			exit(0);
			break;
		default:
			if (map->brush) {
				map->brush->key_press_event(event);
			}
	}
}

void GLWidget::keyReleaseEvent(QKeyEvent* event) {
	input_handler.keys_pressed.erase(event->key());
}

void GLWidget::mouseMoveEvent(QMouseEvent* event) {
	input_handler.mouse_move_event(event);
	camera->mouse_move_event(event);

	if (map->brush) {
		map->brush->mouse_move_event(event);
	}
}

void GLWidget::mousePressEvent(QMouseEvent* event) {
	camera->mouse_press_event(event);
	if (map->brush) {
		map->brush->mouse_press_event(event);
	}
}

void GLWidget::mouseReleaseEvent(QMouseEvent* event) {
	camera->mouse_release_event(event);
	if (map->brush) {
		map->brush->mouse_release_event(event);
	}
}

void GLWidget::wheelEvent(QWheelEvent* event) {
	camera->mouse_scroll_event(event);
}