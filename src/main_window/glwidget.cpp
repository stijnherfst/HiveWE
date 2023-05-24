#include <glad/glad.h>

#include "glwidget.h"

#include "fmt/format.h"

#include <QTimer>
#include <QPainter>

//#include "Globals.h"
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

	setMouseTracking(true);
	setFocus();
	setFocusPolicy(Qt::WheelFocus);
}

std::chrono::steady_clock::time_point begin;


void GLWidget::initializeGL() {
	if (!gladLoadGL()) {
		printf("Something went wrong!\n");
		exit(-1);
	}
	printf("OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);
	
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

	begin = std::chrono::steady_clock::now();
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


void GLWidget::update_scene() {
	delta = elapsed_timer.nsecsElapsed() / 1'000'000'000.0;
	elapsed_timer.start();

	update();
	if (map) {
		map->update(delta, width(), height());
	}

	QTimer::singleShot(16.67 - std::clamp(delta, 0.001, 16.60), this, &GLWidget::update_scene);
}

void GLWidget::paintGL() {
	if (!map) {
		return;
	}

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
		p.drawText(10, 20, QString::fromStdString(fmt::format("Total time: {:.2f}ms", average_frametime * 1000.0)));

		// General info
		p.drawText(300, 20, QString::fromStdString(fmt::format("Mouse Grid Position X:{:.4f} Y:{:.4f}", input_handler.mouse_world.x, input_handler.mouse_world.y)));
		if (map->brush) {
			p.drawText(300, 35, QString::fromStdString(fmt::format("Brush Grid Position X:{:.4f} Y:{:.4f}", map->brush->get_position().x, map->brush->get_position().y)));
		}

		p.drawText(300, 50, QString::fromStdString(fmt::format("Camera Horizontal Angle: {:.4f}", camera.horizontal_angle)));
		p.drawText(300, 64, QString::fromStdString(fmt::format("Camera Vertical Angle: {:.4f}", camera.vertical_angle)));

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
	makeCurrent();

	if (!map) {
		return;
	}

	camera.mouse_press_event(event);
	if (map->brush) {
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