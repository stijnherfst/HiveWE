#pragma once

#include <memory>

#include "MDX.h"

#include "ResourceManager.h"
#include "GPUTexture.h"
#include "Shader.h"


class StaticMesh : public Resource {
public:
	struct MeshEntry {
		int vertices = 0;
		int indices = 0;
		int base_vertex = 0;
		int base_index = 0;

		int material_id = 0;
		bool hd = true;
		mdx::Extent extent;
	};

	std::vector<MeshEntry> geosets;
	bool has_mesh; // ToDo remove when added support for meshless
	mdx::Extent extent;

	GLuint vao;
	GLuint vertex_buffer;
	GLuint uv_buffer;
	GLuint normal_buffer;
	GLuint tangent_buffer;
	GLuint index_buffer;
	GLuint instance_buffer;
	GLuint color_buffer;

	fs::path path;
	std::unordered_map<int, std::shared_ptr<GPUTexture>> textures;
	std::vector<mdx::Material> materials;
	std::vector<glm::mat4> render_jobs;
	std::vector<glm::vec3> render_colors;
	
	static constexpr const char* name = "StaticMesh";

	explicit StaticMesh(const fs::path& path);
	virtual ~StaticMesh();

	const std::string typeToSuffix(mdx::LayerType type);
	void render_queue(const glm::mat4& model, glm::vec3 color);

	void render_opaque_sd() const;
	void render_opaque_hd() const;

	void render_transparent_sd(int instance_id) const;
	void render_transparent_hd(int instance_id) const;

	void render_color_coded(int id, const glm::mat4& matrix);
};