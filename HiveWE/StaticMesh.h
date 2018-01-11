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

	GLuint vertexBuffer;
	GLuint uvBuffer;
	GLuint normalBuffer;
	GLuint indexBuffer;


	int vertices = 0;
	int indices = 0;

	static constexpr const char* name = "Mesh";

	std::vector<std::shared_ptr<GPUTexture>> textures;

	std::shared_ptr<mdx::MTLS> mtls;

	StaticMesh(const std::string& path);

	void render();
};