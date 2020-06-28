#pragma once

#include <vector>

#include <QOpenGLFunctions_4_5_Core>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "SLK2.h"

constexpr int mod_table_write_version = 2;

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
std::vector<std::string> split(const std::string& string, char delimiter);
std::vector<std::string_view> splitSV(std::string_view str, char delimiter);
std::string to_lowercase_copy(const std::string_view& string);
void to_lowercase(std::string& string);

void ltrim(std::string& s);
void rtrim(std::string& s);
void trim(std::string& s);

bool is_number(const std::string& s);

GLuint compile_shader(const fs::path& vertex_shader, const fs::path& fragment_shader);

std::string read_text_file(const fs::path& path);

fs::path find_warcraft_directory();

void load_modification_file(const std::string file_name, slk::SLK2& base_data, slk::SLK2& meta_slk, bool optional_ints);
void load_modification_table(BinaryReader& reader, slk::SLK2& slk, slk::SLK2& meta_slk, bool modification, bool optional_ints);
void save_modification_file(const std::string file_name, slk::SLK2& slk, slk::SLK2& meta_slk, bool optional_ints);
void save_modification_table(BinaryWriter& writer, slk::SLK2& slk, slk::SLK2& meta_slk, bool custom, bool optional_ints);

/// Convert a Tground texture into an QIcon with two states
QIcon ground_texture_to_icon(uint8_t* data, int width, int height);

/// Loads a texture from the hierarchy and returns an icon
QIcon texture_to_icon(fs::path);

extern QOpenGLFunctions_4_5_Core* gl;
extern Shapes shapes;

struct ItemSet {
	std::vector<std::pair<std::string, int>> items;
};

namespace std {
	template<> struct hash<QString> {
		std::size_t operator()(const QString& s) const {
			return qHash(s);
		}
	};
}

extern const glm::vec3 TRANSLATION_IDENTITY;
extern const glm::quat ROTATION_IDENTITY;
extern const glm::vec3 SCALE_IDENTITY;

template <typename T>
void interpolate(T& out, const T* start, const T* outTan, const T* inTan, const T* end, float t, int interpolationType);

/* ToDo replace these with some library calls (glm?), Ghostwolf said it was bad
   practice for me to copy them everywhere (Retera here, also copied them in Matrix Eater)
*/
float lerp(float a, float b, float t);
float hermite(float a, float aOutTan, float bInTan, float b, float t);
float bezier(float a, float aOutTan, float bInTan, float b, float t);
void slerp(glm::quat& out, const glm::quat& startingValue, const glm::quat& endingValue, float interpolationFactor);
void ghostwolfSquad(glm::quat& out, const glm::quat* a, const glm::quat* aOutTan, const glm::quat* bInTan, const glm::quat* b, float interpolationFactor);

float clampValue(float a, float min, float max);

glm::quat safeQuatLookAt(
	glm::vec3 const& lookFrom,
	glm::vec3 const& lookTo,
	glm::vec3 const& up,
	glm::vec3 const& alternativeUp);

void fromRotationTranslationScaleOrigin(const glm::quat& localRotation, const glm::vec3& computedLocation, const glm::vec3& computedScaling, glm::mat4& localMatrix, const glm::vec3& pivot);

void quatMul(const glm::quat& left, const glm::quat& right, glm::quat dest);