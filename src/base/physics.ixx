module;

#include "bullet/btBulletDynamicsCommon.h"
#include <vector>
#include <memory>

#include <glad/glad.h>

export module Physics;

import Shader;
import Camera;
import ResourceManager;

class PhysicsDebugDraw : public btIDebugDraw {
	std::shared_ptr<Shader> shader;
	GLuint vertex_buffer;

  public:
	std::vector<float> debug_vertices;

	explicit PhysicsDebugDraw()	: btIDebugDraw() {
		glCreateBuffers(1, &vertex_buffer);
		shader = resource_manager.load<Shader>({ "Data/Shaders/physics_debug.vert", "Data/Shaders/physics_debug.frag" });
	}

	void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
		debug_vertices.push_back(from.x());
		debug_vertices.push_back(from.y());
		debug_vertices.push_back(from.z());
		debug_vertices.push_back(to.x());
		debug_vertices.push_back(to.y());
		debug_vertices.push_back(to.z());
	}

	void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {
	}

	void reportErrorWarning(const char* warningString) {
	}

	void draw3dText(const btVector3& location, const char* textString) {
	}

	void setDebugMode(int debugMode) {
	}

	int getDebugMode() const {
		return DBG_DrawWireframe;
	}

	void clearLines() {
		debug_vertices.clear();
	}

	void render() {
		glNamedBufferData(vertex_buffer, debug_vertices.size() * sizeof(float), debug_vertices.data(), GL_STATIC_DRAW);

		shader->use();
		// glDisable(GL_DEPTH_TEST);

		glUniformMatrix4fv(1, 1, GL_FALSE, &camera.projection_view[0][0]);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glDrawArrays(GL_LINES, 0, debug_vertices.size());

		glDisableVertexAttribArray(0);
		glEnable(GL_DEPTH_TEST);
	}
};

export struct Physics {
	float gravity = -9.81f;

	btBroadphaseInterface* broadphase;
	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* dynamicsWorld;

	PhysicsDebugDraw* draw;

	explicit Physics() {
		broadphase = new btDbvtBroadphase();
		collisionConfiguration = new btDefaultCollisionConfiguration();
		dispatcher = new btCollisionDispatcher(collisionConfiguration);
		solver = new btSequentialImpulseConstraintSolver;
		dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
		dynamicsWorld->setGravity(btVector3(0, 0, gravity));

		draw = new PhysicsDebugDraw;
		draw->setDebugMode(draw->getDebugMode() | btIDebugDraw::DBG_DrawAabb);
		dynamicsWorld->setDebugDrawer(draw);
	}

	~Physics() {
		delete dynamicsWorld;
		delete solver;
		delete dispatcher;
		delete collisionConfiguration;
		delete broadphase;
		delete draw;
	}
};