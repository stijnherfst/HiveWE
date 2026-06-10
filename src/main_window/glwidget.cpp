#include "glwidget.h"

#include <QTimer>
#include <QPainter>

#include <tracy/Tracy.hpp>

import std;
import OpenGLUtilities;
import Camera;
import MapGlobal;
import <glad/glad.h>;
import <glm/glm.hpp>;

void APIENTRY gl_debug_output(
	const GLenum source,
	const GLenum type,
	const GLuint id,
	const GLenum severity,
	const GLsizei,
	const GLchar* message,
	void*
) {
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
		case GL_DEBUG_SOURCE_API:
			std::println("Source: API");
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			std::println("Source: Window System");
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			std::println("Source: Shader Compiler");
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			std::println("Source: Third Party");
			break;
		case GL_DEBUG_SOURCE_APPLICATION:
			std::println("Source: Application");
			break;
		case GL_DEBUG_SOURCE_OTHER:
			std::println("Source: Other");
			break;
		default:
			break;
	}

	switch (type) {
		case GL_DEBUG_TYPE_ERROR:
			std::println("Type: Error");
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			std::println("Type: Deprecated Behaviour");
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			std::println("Type: Undefined Behaviour");
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			std::println("Type: Portability");
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			std::println("Type: Performance");
			break;
		case GL_DEBUG_TYPE_MARKER:
			std::println("Type: Marker");
			break;
		case GL_DEBUG_TYPE_PUSH_GROUP:
			std::println("Type: Push Group");
			break;
		case GL_DEBUG_TYPE_POP_GROUP:
			std::println("Type: Pop Group");
			break;
		case GL_DEBUG_TYPE_OTHER:
			std::println("Type: Other");
			break;
		default:
			break;
	}

	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:
			std::println("Severity: high");
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			std::println("Severity: medium");
			break;
		case GL_DEBUG_SEVERITY_LOW:
			std::println("Severity: low");
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			std::println("Severity: notification");
			break;
		default:
			break;
	}
}

GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent) {
	setMouseTracking(true);
	setFocus();
	setFocusPolicy(Qt::WheelFocus);

	connect(this, &QOpenGLWidget::frameSwapped, [&]() {
		update();
	});
}

void GLWidget::initializeGL() {
	if (!gladLoadGL()) {
		std::println("Something went wrong initializing GLAD");
		exit(-1);
	}
	std::println("OpenGL {}.{}", GLVersion.major, GLVersion.minor);

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(gl_debug_output), nullptr);
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
	camera.aspect_ratio = static_cast<double>(w) / h;

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
		QFont font("Consolas");
		font.setStyleHint(QFont::Monospace);
		p.setFont(font);

		// Rendering time
		static std::vector<double> frametimes;
		frametimes.push_back(delta);
		if (frametimes.size() > 60) {
			frametimes.erase(frametimes.begin());
		}
		float average_frametime = std::accumulate(frametimes.begin(), frametimes.end(), 0.f) / frametimes.size();
		p.drawText(10, 20, QString::fromStdString(std::format("Total time: {:.2f}ms", average_frametime * 1000.0)));

		// General info
		auto fmt_vec3 = [](const char* label, glm::vec3 v) {
			return std::format("{:<20} X:{:>6.3f}  Y:{:>6.3f}  Z:{:>6.3f}", label, v.x, v.y, v.z);
		};

		auto fmt_vec2 = [](const char* label, glm::vec2 v) {
			return std::format("{:<20} X:{:>6.3f}  Y:{:>6.3f}", label, v.x, v.y);
		};

		p.drawText(175, 20, QString::fromStdString(fmt_vec3("Mouse World Position", input_handler.mouse_world)));
		p.drawText(175, 35, QString::fromStdString(fmt_vec3("Camera Position", camera.position)));

		if (map->brush) {
			p.drawText(175, 50, QString::fromStdString(fmt_vec2("Brush Grid Position", map->brush->get_position())));
		}

		p.drawText(175, 65, QString::fromStdString(std::format("Camera Horizontal Angle: {:.4f}", camera.horizontal_angle)));
		p.drawText(175, 80, QString::fromStdString(std::format("Camera Vertical Angle: {:.4f}", camera.vertical_angle)));

		const glm::ivec2 terrain_index = input_handler.mouse_world;
		const auto corner = map->terrain.get_corner(terrain_index.x, terrain_index.y);
		p.drawText(550, 20, QString::fromStdString(std::format("Tile info")));
		p.drawText(550, 35, QString::fromStdString(std::format("Cliff: {}", corner.cliff)));
		p.drawText(550, 50, QString::fromStdString(std::format("Blight: {}", corner.blight)));
		p.drawText(550, 65, QString::fromStdString(std::format("Boundary: {}", corner.boundary)));
		p.drawText(550, 80, QString::fromStdString(std::format("Cliff texture: {}", corner.cliff_texture)));
		p.drawText(550, 95, QString::fromStdString(std::format("Cliff variation: {}", corner.cliff_variation)));
		p.drawText(550, 110, QString::fromStdString(std::format("Ground texture: {}", corner.ground_texture)));
		p.drawText(550, 125, QString::fromStdString(std::format("Ground variation: {}", corner.ground_variation)));
		p.drawText(550, 140, QString::fromStdString(std::format("Height: {}", corner.height)));
		p.drawText(550, 155, QString::fromStdString(std::format("Layer height: {}", corner.layer_height)));
		p.drawText(550, 170, QString::fromStdString(std::format("Map edge: {}", corner.map_edge)));
		p.drawText(550, 185, QString::fromStdString(std::format("Ramp: {}", corner.ramp)));
		p.drawText(550, 200, QString::fromStdString(std::format("Romp: {}", corner.romp)));
		p.drawText(550, 215, QString::fromStdString(std::format("Special doodad: {}", corner.special_doodad)));
		p.drawText(550, 230, QString::fromStdString(std::format("Water: {}", corner.water)));
		p.drawText(550, 245, QString::fromStdString(std::format("Water height: {}", corner.water_height)));

		p.end();

		// Set changed state back
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	FrameMark;
}

void GLWidget::keyPressEvent(QKeyEvent* event) {
	if (!map) {
		return;
	}

	input_handler.keys_pressed.emplace(event->key());

	if (map->brush) {
		map->brush->key_press_event(event);
	}
	QOpenGLWidget::keyPressEvent(event);
}

void GLWidget::keyReleaseEvent(QKeyEvent* event) {
	if (!map) {
		return;
	}

	input_handler.keys_pressed.erase(event->key());

	if (map->brush) {
		map->brush->key_release_event(event);
	}
	QOpenGLWidget::keyReleaseEvent(event);
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

	if (event->modifiers() & Qt::ShiftModifier && map->brush) {
		// Some platforms report the delta on the x-axis when shift is held
		const int delta = event->angleDelta().y() != 0 ? event->angleDelta().y() : event->angleDelta().x();
		if (delta > 0) {
			map->brush->increase_size(1);
		} else if (delta < 0) {
			map->brush->decrease_size(1);
		}
		return;
	}

	camera.mouse_scroll_event(event);
}
