#pragma once

#include <vector>
#include <filesystem>
#include <QOpenGLFunctions_4_5_Core>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
import BinaryReader;
import BinaryWriter;
#include "SLK.h"

constexpr int mod_table_write_version = 3;
namespace fs = std::filesystem;

class Shapes {
public:
	void init();

	GLuint vertex_buffer;
	GLuint index_buffer;

	const std::vector<glm::vec2> quad_vertices = {
		{ 1, 1 },
		{ 0, 1 },
		{ 0, 0 },
		{ 1, 0 }
	};

	const std::vector<glm::uvec3> quad_indices = {
		{ 0, 1, 2 },
		{ 2, 3, 0 }
	};
};

// String functions
std::string string_replaced(const std::string& source, const std::string& from, const std::string& to);

std::string to_lowercase_copy(const std::string_view& string);
void to_lowercase(std::string& string);

void ltrim(std::string& s);
void rtrim(std::string& s);
void trim(std::string& s);

bool is_number(const std::string& s);

GLuint compile_shader(const fs::path& vertex_shader, const fs::path& fragment_shader);

std::string read_text_file(const fs::path& path);

fs::path find_warcraft_directory();

void load_modification_file(const std::string file_name, slk::SLK& base_data, slk::SLK& meta_slk, bool optional_ints);
void load_modification_table(BinaryReader& reader, uint32_t version, slk::SLK& slk, slk::SLK& meta_slk, bool modification, bool optional_ints);
void save_modification_file(const std::string file_name, slk::SLK& slk, slk::SLK& meta_slk, bool optional_ints, bool skin);
void save_modification_table(BinaryWriter& writer, slk::SLK& slk, slk::SLK& meta_slk, bool custom, bool optional_ints, bool skin);

/// Convert a Tground texture into an QIcon with two states
QIcon ground_texture_to_icon(uint8_t* data, int width, int height);

/// Loads a texture from the hierarchy and returns an icon
QIcon texture_to_icon(fs::path);

extern QOpenGLFunctions_4_5_Core* gl;
extern Shapes shapes;

struct ItemSet {
	std::vector<std::pair<std::string, int>> items;
};

extern const glm::vec3 TRANSLATION_IDENTITY;
extern const glm::quat ROTATION_IDENTITY;
extern const glm::vec3 SCALE_IDENTITY;

float interpolate(const float start, const float outTan, const float inTan, const float end, float t, int interpolationType);
glm::vec3 interpolate(const glm::vec3 start, const glm::vec3 outTan, const glm::vec3 inTan, const glm::vec3 end, float t, int interpolationType);
glm::quat interpolate(const glm::quat start, const glm::quat outTan, const glm::quat inTan, const glm::quat end, float t, int interpolationType);
uint32_t interpolate(const uint32_t start, const uint32_t outTan, const uint32_t inTan, const uint32_t end, float t, int interpolationType);

/* ToDo replace these with some library calls (glm?), Ghostwolf said it was bad
   practice for me to copy them everywhere (Retera here, also copied them in Matrix Eater)
*/
float hermite(float a, float aOutTan, float bInTan, float b, float t);
float bezier(float a, float aOutTan, float bInTan, float b, float t);
glm::quat ghostwolfSquad(const glm::quat a, const glm::quat aOutTan, const glm::quat bInTan, const glm::quat b, float interpolationFactor);

glm::quat safeQuatLookAt(
	glm::vec3 const& lookFrom,
	glm::vec3 const& lookTo,
	glm::vec3 const& up,
	glm::vec3 const& alternativeUp);

void fromRotationTranslationScaleOrigin(const glm::quat& localRotation, const glm::vec3& computedLocation, const glm::vec3& computedScaling, glm::mat4& localMatrix, const glm::vec3& pivot);