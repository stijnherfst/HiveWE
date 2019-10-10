#include "physics.h"

#include "Camera.h"

PhysicsDebugDraw::PhysicsDebugDraw() : btIDebugDraw() {
	gl->glCreateBuffers(1, &vertex_buffer);
	shader = resource_manager.load<Shader>({ "Data/Shaders/physics_debug.vs", "Data/Shaders/physics_debug.fs" });
}

void PhysicsDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
	debug_vertices.push_back(from.x());
	debug_vertices.push_back(from.y());
	debug_vertices.push_back(from.z());
	debug_vertices.push_back(to.x());
	debug_vertices.push_back(to.y());
	debug_vertices.push_back(to.z());
}

void PhysicsDebugDraw::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {
}

void PhysicsDebugDraw::reportErrorWarning(const char* warningString) {}

void PhysicsDebugDraw::draw3dText(const btVector3& location, const char* textString) {
}

void PhysicsDebugDraw::setDebugMode(int debugMode) {}

int PhysicsDebugDraw::getDebugMode() const {
	return DBG_DrawWireframe;
}

void PhysicsDebugDraw::clearLines() {
	debug_vertices.clear();
}

void PhysicsDebugDraw::render() {
	gl->glNamedBufferData(vertex_buffer, debug_vertices.size() * sizeof(float), debug_vertices.data(), GL_STATIC_DRAW);

	shader->use();
	//gl->glDisable(GL_DEPTH_TEST);

	gl->glUniformMatrix4fv(1, 1, GL_FALSE, &camera->projection_view[0][0]);

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glDrawArrays(GL_LINES, 0, debug_vertices.size());

	gl->glDisableVertexAttribArray(0);
	gl->glEnable(GL_DEPTH_TEST);
}

void Physics::initialize() {
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

Physics physics;