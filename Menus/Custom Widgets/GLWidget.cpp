#include "GLWidget.h"

#include "fmt/format.h"

#include <QTimer>
#include <QOpenGLFunctions_4_5_Core>
#include <QPainter>

#include "Utilities.h"
#include "InputHandler.h"

#include "HiveWE.h"

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
	setFocusPolicy(Qt::WheelFocus);
}

void GLWidget::initializeGL() {
	gl = new QOpenGLFunctions_4_5_Core;
	gl->initializeOpenGLFunctions();

	gl->glEnable(GL_DEBUG_OUTPUT);
	gl->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	gl->glDebugMessageCallback(GLDEBUGPROC(gl_debug_output), nullptr);
	gl->glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, false);
	gl->glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, true);
	gl->glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, true);
	gl->glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, true);

	gl->glEnable(GL_DEPTH_TEST);
	gl->glEnable(GL_CULL_FACE);
	gl->glDepthFunc(GL_LEQUAL);
	gl->glEnable(GL_BLEND);
	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gl->glClearColor(0, 0, 0, 1);

	gl->glGenVertexArrays(1, &vao);
	gl->glBindVertexArray(vao);


	int extension_count;
	gl->glGetIntegerv(GL_NUM_EXTENSIONS, &extension_count);

	shapes.init();
}

void GLWidget::resizeGL(const int w, const int h) {
	gl->glViewport(0, 0, w, h);

	const double delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	camera->aspect_ratio = double(w) / h;

	if (!map || !map->loaded) {
		return;
	}
	camera->update(delta);
	map->render_manager.resize_framebuffers(w, h);
}

void GLWidget::update_scene() {
	delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	elapsed_timer.start();

	update();
	if (map) {
		map->update(delta, width(), height());
		QTimer::singleShot(std::max(0.0, 16.0 - delta), this, &GLWidget::update_scene);
	} else {
		QTimer::singleShot(std::max(0.0, 16.0), this, &GLWidget::update_scene);
	}
}

void GLWidget::paintGL() {
	if (!map) {
		return;
	}

	gl->glEnable(GL_DEPTH_TEST);
	gl->glDepthMask(true);
	gl->glClearColor(0, 0, 0, 1);
	gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	gl->glBindVertexArray(vao);
	map->render();

	gl->glBindVertexArray(0);

	if (map->render_debug) {
		QPainter p(this);
		p.setPen(QColor(Qt::GlobalColor::white));
		p.setFont(QFont("Arial", 10, 100, false));

		// Rendering time
		static std::vector<double> frametimes;
		frametimes.push_back(delta * 1000.f);
		if (frametimes.size() > 60) {
			frametimes.erase(frametimes.begin());
		}
		float average_frametime = std::accumulate(frametimes.begin(), frametimes.end(), 0.f) / frametimes.size();
		p.drawText(10, 20, QString::fromStdString(fmt::format("Total time: {:.4f}ms", average_frametime)));

		// General info
		p.drawText(300, 20, QString::fromStdString(fmt::format("Mouse Grid Position X:{:.4f} Y:{:.4f}", input_handler.mouse_world.x, input_handler.mouse_world.y)));
		if (map->brush) {
			p.drawText(300, 35, QString::fromStdString(fmt::format("Brush Grid Position X:{:.4f} Y:{:.4f}", map->brush->get_position().x, map->brush->get_position().y)));
		}

		p.drawText(300, 50, QString::fromStdString(fmt::format("Camera Horizontal Angle: {:.4f}", camera->horizontal_angle)));
		p.drawText(300, 64, QString::fromStdString(fmt::format("Camera Vertical Angle: {:.4f}", camera->vertical_angle)));

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
	input_handler.keys_pressed.emplace(event->key());
	switch (event->key()) {
		//case Qt::Key_Escape:
		//	exit(0);
		//	break;
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

	if (map->brush) {
		map->brush->key_release_event(event);
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
	makeCurrent();
	gl->glBindVertexArray(vao);
	gl->glViewport(0, 0, width(), height());

	if (!map) {
		return;
	}

	camera->mouse_press_event(event);
	if (map->brush) {
		map->brush->mouse_press_event(event);
	}
	gl->glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
	gl->glBindVertexArray(0);
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