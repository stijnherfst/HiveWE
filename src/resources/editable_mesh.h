#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <filesystem>
namespace fs = std::filesystem;

#include <glad/glad.h>

import MDX;
import ResourceManager;
import GPUTexture;
import Shader;
import SkeletalModelInstance;

class EditableMesh : public Resource {
  public:
	struct MeshEntry {
		int vertices = 0;
		int indices = 0;
		int base_vertex = 0;
		int base_index = 0;

		int material_id = 0;
		mdx::Extent extent;

		bool hd = true;
		mdx::GeosetAnimation* geoset_anim; // can be nullptr, often
	};

	std::shared_ptr<mdx::MDX> mdx;

	std::vector<MeshEntry> geosets;
	bool has_mesh; // ToDo remove when added support for meshless

	GLuint vao;
	GLuint vertex_buffer;
	GLuint uv_buffer;
	GLuint normal_buffer;
	GLuint tangent_buffer;
	GLuint weight_buffer;
	GLuint index_buffer;
	GLuint layer_alpha;
	GLuint geoset_color;

	fs::path path;
	std::vector<std::shared_ptr<GPUTexture>> textures;

	static constexpr const char* name = "EditableMesh";

	explicit EditableMesh(const fs::path& path, std::optional<std::pair<int, std::string>> replaceable_id_override);
	virtual ~EditableMesh();

	void render(const SkeletalModelInstance& skeleton, const glm::mat4 projection_view, glm::vec3 light_direction);

private:
	//void render_queue(const SkeletalModelInstance& skeleton, glm::vec3 color);
	//void render_color_coded(const SkeletalModelInstance& skeleton, int id);

	//void render_opaque_sd();
  void render_opaque_hd(const SkeletalModelInstance& skeleton, const glm::mat4 projection_view, glm::vec3 light_direction);

	//void render_transparent_sd(int instance_id);
	//void render_transparent_hd(int instance_id);
};