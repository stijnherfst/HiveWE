#include "stdafx.h"

void APIENTRY gl_debug_output(const GLenum source, const GLenum type, const GLuint id, const GLenum severity, const GLsizei, const GLchar *message, void *) {
	// Skip buffer info messages, framebuffer info messages, texture usage state warning, redundant state change buffer
	if (id == 131185 // ?
		|| id == 131169 // ?
		|| id == 131204 // ?
		|| id == 8 // ?
		|| id == 131218) // Unexplainable performance warnings
	{
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
}

void GLWidget::resizeGL(const int w, const int h) {
	gl->glViewport(0, 0, w, h);

	const double delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	camera->aspect_ratio = double(w) / h;

	if (!map) {
		return;
	}
	camera->update(delta);
}

void GLWidget::update_scene() {
	const double delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	elapsed_timer.start();

	if (map) {
		camera->update(delta);

		map->terrain.current_texture += std::max(0.0, map->terrain.animation_rate * delta);
		if (map->terrain.current_texture >= map->terrain.water_textures_nr) {
			map->terrain.current_texture = 0;
		}
	}

	update();
	if (map) {
		QTimer::singleShot(std::max(0.0, 16.0 - map->total_time), this, &GLWidget::update_scene);
	} else {
		QTimer::singleShot(std::max(0.0, 16.0), this, &GLWidget::update_scene);
	}
}

void GLWidget::paintGL() {
	if (!map) {
		return;
	}

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
		p.drawText(10, 20, QString::fromStdString("Total time: " + std::to_string(map->total_time)));

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
	if (!map) {
		return;
	}

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
	if (!map) {
		return;
	}

	input_handler.keys_pressed.erase(event->key());
}

void GLWidget::mouseMoveEvent(QMouseEvent* event) {
	if (!map) {
		return;
	}

	input_handler.mouse_move_event(event);
	camera->mouse_move_event(event);

	if (map->brush) {
		map->brush->mouse_move_event(event);
	}
}

void GLWidget::mousePressEvent(QMouseEvent* event) {
	if (!map) {
		return;
	}

	camera->mouse_press_event(event);
	if (map->brush) {
		map->brush->mouse_press_event(event);
	}
}

void GLWidget::mouseReleaseEvent(QMouseEvent* event) {
	if (!map) {
		return;

	}
	camera->mouse_release_event(event);
	if (map->brush) {
		map->brush->mouse_release_event(event);
	}
}

void GLWidget::wheelEvent(QWheelEvent* event) {
	if (!map) {
		return;
	}

	camera->mouse_scroll_event(event);
}