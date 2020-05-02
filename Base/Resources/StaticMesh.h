#pragma once

#include <memory>

#include "MDX.h"

#include "ResourceManager.h"
#include "GPUTexture.h"
#include "Shader.h"

struct Animation {
	uint32_t interval_start;
	uint32_t interval_end;
	float movespeed;
	uint32_t flags; // 0: looping
					// 1: non looping
	float rarity;
	uint32_t sync_point;
	mdx::Extent extent;
};

class StaticMesh : public Resource {
public:
	struct MeshEntry {
		int vertices = 0;
		int indices = 0;
		int base_vertex = 0;
		int base_index = 0;

		int material_id = 0;
		bool visible = true;
	};

	std::vector<MeshEntry> entries;
	bool has_mesh; // ToDo remove when added support for meshless

	std::map<std::string, Animation> animations;

	GLuint vertex_buffer;
	GLuint uv_buffer;
	GLuint normal_buffer;
	GLuint index_buffer;
	GLuint instance_buffer;

	fs::path path;

	std::vector<std::shared_ptr<GPUTexture>> textures;
	std::shared_ptr<mdx::MTLS> mtls;
	std::shared_ptr<Shader> shader; // ToDo only needed one for class
	std::vector<glm::mat4> render_jobs;
	
	static constexpr const char* name = "StaticMesh";


	explicit StaticMesh(const fs::path& path);
	virtual ~StaticMesh();

	void render_queue(const glm::mat4& mvp);
	void render();
};