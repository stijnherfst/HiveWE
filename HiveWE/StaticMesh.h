#pragma once

class StaticMesh : public Resource {
public:
	struct MeshEntry {
		int vertices = 0;
		int indices = 0;
		int base_vertex = 0;
		int base_index = 0;

		int material_id;
	};

	std::vector<MeshEntry> entries;
	bool has_mesh; // ToDo remove when added support for meshless

	GLuint vertexBuffer;
	GLuint uvBuffer;
	GLuint normalBuffer;
	GLuint indexBuffer;
	GLuint instanceBuffer;

	int vertices = 0;
	int indices = 0;

	std::vector<std::shared_ptr<GPUTexture>> textures;
	std::shared_ptr<mdx::MTLS> mtls;
	std::shared_ptr<Shader> shader;
	std::shared_ptr<Shader> shader2;
	
	static constexpr const char* name = "Mesh";

	std::vector<glm::mat4> render_jobs;

	StaticMesh(const std::string& path);
	virtual ~StaticMesh();

	void render_queue(glm::mat4 mvp);
	void render();
};