#include <glad/glad.h>

#include <print>
#include <format>

#include "glwidget.h"

#include <QTimer>
#include <QPainter>
#include <glad/glad.h>

#include <map_global.h>

import OpenGLUtilities;
import Camera;

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

	std::println("---------------");
	std::println("Debug message ({})", message);

	switch (source) {
		case GL_DEBUG_SOURCE_API:             std::println("Source: API"); break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::println("Source: Window System"); break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::println("Source: Shader Compiler"); break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::println("Source: Third Party"); break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::println("Source: Application"); break;
		case GL_DEBUG_SOURCE_OTHER:           std::println("Source: Other"); break;
		default: break;
	}

	switch (type) {
		case GL_DEBUG_TYPE_ERROR:               std::println("Type: Error"); break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::println("Type: Deprecated Behaviour"); break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::println("Type: Undefined Behaviour"); break;
		case GL_DEBUG_TYPE_PORTABILITY:         std::println("Type: Portability"); break;
		case GL_DEBUG_TYPE_PERFORMANCE:         std::println("Type: Performance"); break;
		case GL_DEBUG_TYPE_MARKER:              std::println("Type: Marker"); break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          std::println("Type: Push Group"); break;
		case GL_DEBUG_TYPE_POP_GROUP:           std::println("Type: Pop Group"); break;
		case GL_DEBUG_TYPE_OTHER:               std::println("Type: Other"); break;
		default: break;
	}

	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:         std::println("Severity: high"); break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::println("Severity: medium"); break;
		case GL_DEBUG_SEVERITY_LOW:          std::println("Severity: low"); break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::println("Severity: notification"); break;
		default: break;
	}
}

GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent) {
	setMouseTracking(true);
	setFocus();
	setFocusPolicy(Qt::WheelFocus);

	connect(this, &QOpenGLWidget::frameSwapped, [&]() { update(); });
}

void GLWidget::initializeGL() {
	if (!gladLoadGL()) {
		std::println("Something went wrong initializing GLAD");
		exit(-1);
	}
	std::println("OpenGL {}.{}", GLVersion.major, GLVersion.minor);
	
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(GLDEBUGPROC(gl_debug_output), nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, false);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, true);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, true);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, true);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0, 0, 0, 1);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	int extension_count;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extension_count);
	
	shapes.init();
}

void GLWidget::resizeGL(const int w, const int h) {
	glViewport(0, 0, w, h);

	delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	camera.aspect_ratio = double(w) / h;

	if (!map || !map->loaded) {
		return;
	}
	camera.update(delta);
	map->render_manager.resize_framebuffers(w, h);
}

void GLWidget::paintGL() {
	delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	elapsed_timer.start();

	if (!map) {
		return;
	}

	map->update(delta, width(), height());

	//glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(true);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(vao);
	map->render();

	glBindVertexArray(0);

	if (map->render_debug) {
		QPainter p(this);
		p.setPen(QColor(Qt::GlobalColor::white));
		p.setFont(QFont("Arial", 10, 100, false));

		// Rendering time
		static std::vector<double> frametimes;
		frametimes.push_back(delta);
		if (frametimes.size() > 60) {
			frametimes.erase(frametimes.begin());
		}
		float average_frametime = std::accumulate(frametimes.begin(), frametimes.end(), 0.f) / frametimes.size();
		p.drawText(10, 20, QString::fromStdString(std::format("Total time: {:.2f}ms", average_frametime * 1000.0)));

		// General info
		p.drawText(300, 20, QString::fromStdString(std::format("Mouse World Position X:{:.4f} Y:{:.4f} Z:{:.4f}", input_handler.mouse_world.x, input_handler.mouse_world.y, input_handler.mouse_world.z)));
		p.drawText(300, 35, QString::fromStdString(std::format("Camera Position X:{:.4f} Y:{:.4f} Z:{:.4f}", camera.position.x, camera.position.y, camera.position.z)));
		if (map->brush) {
			p.drawText(300, 50, QString::fromStdString(std::format("Brush Grid Position X:{:.4f} Y:{:.4f}", map->brush->get_position().x, map->brush->get_position().y)));
		}

		p.drawText(300, 65, QString::fromStdString(std::format("Camera Horizontal Angle: {:.4f}", camera.horizontal_angle)));
		p.drawText(300, 80, QString::fromStdString(std::format("Camera Vertical Angle: {:.4f}", camera.vertical_angle)));

		p.end();

		// Set changed state back
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void GLWidget::keyPressEvent(QKeyEvent* event) {
	if (!map) {
		return;
	}

	input_handler.keys_pressed.emplace(event->key());

	if (map->brush) {
		map->brush->key_press_event(event);
	}
}

void GLWidget::keyReleaseEvent(QKeyEvent* event) {
	if (!map) {
		return;
	}

	input_handler.keys_pressed.erase(event->key());

	if (map->brush) {
		map->brush->key_release_event(event);
	}
}

void GLWidget::mouseMoveEvent(QMouseEvent* event) {
	if (!map) {
		return;
	}

	input_handler.mouse_move_event(event);
	camera.mouse_move_event(event);

	if (map->brush) {
		map->brush->mouse_move_event(event, delta);
	}
}

void GLWidget::mousePressEvent(QMouseEvent* event) {

	if (!map) {
		return;
	}

	camera.mouse_press_event(event);
	if (map->brush) {
		makeCurrent();
		map->brush->mouse_press_event(event, delta);
	}
}

void GLWidget::mouseReleaseEvent(QMouseEvent* event) {
	if (!map) {
		return;

	}
	camera.mouse_release_event(event);
	if (map->brush) {
		map->brush->mouse_release_event(event);
	}
}

void GLWidget::wheelEvent(QWheelEvent* event) {
	if (!map) {
		return;
	}

	camera.mouse_scroll_event(event);
}