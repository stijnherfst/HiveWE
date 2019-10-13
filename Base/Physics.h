//#pragma once
//
//#include "bullet/btBulletDynamicsCommon.h"
//#include <vector>
//#include "Shader.h"
//#include <memory>
//
//class PhysicsDebugDraw : public btIDebugDraw {
//	std::shared_ptr<Shader> shader;
//	GLuint vertex_buffer;
//
//
//public:
//	std::vector<float> debug_vertices;
//	PhysicsDebugDraw();
//
//	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
//	virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
//	virtual void reportErrorWarning(const char* warningString) override;
//	virtual void draw3dText(const btVector3& location, const char* textString) override;
//	virtual void setDebugMode(int debugMode) override;
//	virtual int  getDebugMode() const override;
//	virtual void clearLines() override;
//	void render();
//};
//
//struct Physics {
//	float gravity = -9.81f;
//
//	btBroadphaseInterface* broadphase;
//	btDefaultCollisionConfiguration* collisionConfiguration;
//	btCollisionDispatcher* dispatcher;
//	btSequentialImpulseConstraintSolver* solver;
//	btDiscreteDynamicsWorld* dynamicsWorld;
//
//	
//	PhysicsDebugDraw* draw;
//	
//	Physics();
//
//	~Physics() {
//		delete broadphase;
//		delete collisionConfiguration;
//		delete dispatcher;
//		delete solver;
//		delete dynamicsWorld;
//		delete draw;
//	}
//};