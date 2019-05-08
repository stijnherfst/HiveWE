#pragma once

class SkinnedMesh : public Resource {
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
	std::map<int, mdx::Bone> id_to_bone;

	GLuint vertex_buffer;
	GLuint uv_buffer;
	GLuint normal_buffer;
	GLuint index_buffer;

	fs::path path;

	std::vector<std::shared_ptr<GPUTexture>> textures;
	std::shared_ptr<mdx::MTLS> mtls;
	std::shared_ptr<Shader> shader; // ToDo only needed one per class

	static constexpr const char* name = "SkinnedMesh";


	explicit SkinnedMesh(const fs::path& path);
	virtual ~SkinnedMesh();

	void render();
};