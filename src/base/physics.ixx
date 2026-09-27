export module Physics;

import std;
import <bullet/btBulletDynamicsCommon.h>;

/// Collects the lines Bullet's debug drawing produces
class PhysicsDebugDraw : public btIDebugDraw {
  public:
	std::vector<float> debug_vertices;

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

};

export struct Physics {
	float gravity = -9.81f;

	btBroadphaseInterface* broadphase;
	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* dynamicsWorld;

	// PhysicsDebugDraw* draw;

	explicit Physics() {
		broadphase = new btDbvtBroadphase();
		collisionConfiguration = new btDefaultCollisionConfiguration();
		dispatcher = new btCollisionDispatcher(collisionConfiguration);
		solver = new btSequentialImpulseConstraintSolver;
		dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
		dynamicsWorld->setGravity(btVector3(0, 0, gravity));

		// draw = new PhysicsDebugDraw;
		// draw->setDebugMode(draw->getDebugMode() | btIDebugDraw::DBG_DrawAabb);
		// dynamicsWorld->setDebugDrawer(draw);
	}

	~Physics() {
		delete dynamicsWorld;
		delete solver;
		delete dispatcher;
		delete collisionConfiguration;
		delete broadphase;
		// delete draw;
	}
};