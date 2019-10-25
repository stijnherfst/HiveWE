#include <vector>

#include "StaticMesh.h"

class RenderManager {
public:
	struct Inst {
		int mesh_id;
		int instance_id;
		float distance;
	};

	std::shared_ptr<Shader> instance_static_mesh_shader;
	std::shared_ptr<Shader> static_mesh_shader;

	std::vector<StaticMesh*> meshes;
	std::vector<Inst> transparent_instances;
	RenderManager();

	void render();
};