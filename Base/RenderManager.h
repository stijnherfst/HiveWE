#include <vector>

#include "StaticMesh.h"
#include "SkinnedMesh.h"

class RenderManager {
public:
	struct Inst {
		int mesh_id;
		int instance_id;
		float distance;
	};

	std::shared_ptr<Shader> instance_static_mesh_shader_sd;
	std::shared_ptr<Shader> instance_static_mesh_shader_hd;
	std::shared_ptr<Shader> static_mesh_shader_sd;
	std::shared_ptr<Shader> static_mesh_shader_hd;

	std::shared_ptr<Shader> instance_skinned_mesh_shader;
	std::shared_ptr<Shader> skinned_mesh_shader;

	std::vector<StaticMesh*> meshes;
	std::vector<SkinnedMesh*> animated_meshes;
	std::vector<Inst> transparent_instances;
	std::vector<Inst> skinned_transparent_instances;
	RenderManager();

	void render(bool render_lighting);
};