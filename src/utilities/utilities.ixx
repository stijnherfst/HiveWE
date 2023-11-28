module;

#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <print>

#include <glm/glm.hpp>

export module Utilities;

namespace fs = std::filesystem;

// String functions
export std::string string_replaced(const std::string& source, const std::string& from, const std::string& to) {
	std::string new_string;
	new_string.reserve(source.length()); // avoids a few memory allocations

	size_t lastPos = 0;
	size_t findPos;

	while (std::string::npos != (findPos = source.find(from, lastPos))) {
		new_string.append(source, lastPos, findPos - lastPos);
		new_string += to;
		lastPos = findPos + from.length();
	}

	// Care for the rest after last occurrence
	new_string += source.substr(lastPos);

	return new_string;
}

export std::string to_lowercase_copy(const std::string_view& string) {
	std::string output(string);
	std::transform(output.begin(), output.end(), output.begin(), [](unsigned char c) { return std::tolower(c); });
	return output;
}

export void to_lowercase(std::string& string) {
	std::transform(string.begin(), string.end(), string.begin(), [](unsigned char c) { return std::tolower(c); });
}

// trim from start (in place)
export void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
				return !std::isspace(ch);
			}));
}

// trim from end (in place)
export void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
				return !std::isspace(ch);
			}).base(),
			s.end());
}

// trim from both ends (in place)
export void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}

export bool is_number(const std::string& s) {
	return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

export std::string read_text_file(const fs::path& path) {
	std::ifstream textfile(path.c_str());
	std::string line;
	std::string text;

	if (!textfile.is_open())
		return "";

	while (getline(textfile, line)) {
		text += line + "\n";
	}

	return text;
}

export struct ItemSet {
	std::vector<std::pair<int, std::string>> items;
};

// Returns 1 or -1
export glm::vec2 sign_not_zero(glm::vec2 v) {
	return glm::vec2((v.x >= 0.f) ? +1.f : -1.f, (v.y >= 0.f) ? +1.f : -1.f);
}
// Assume normalized input. Output is on [-1, 1] for each component.
export glm::vec2 float32x3_to_oct(glm::vec3 v) {
	// Project the sphere onto the octahedron, and then onto the xy plane
	glm::vec2 p = glm::vec2(v) * (1.f / (std::abs(v.x) + std::abs(v.y) + std::abs(v.z)));
	// Reflect the folds of the lower hemisphere over the diagonals
	return (v.z <= 0.f) ? ((1.f - glm::abs(glm::vec2(p.y, p.x))) * sign_not_zero(p)) : p;
}

// From http://www.jcgt.org/published/0006/02/01/
export bool intersect_aabb(const glm::vec3& aabb_min, const glm::vec3& aabb_max, const glm::vec3& origin, const glm::vec3& direction) {
	const glm::vec3 t1 = (aabb_min - origin) / direction;
	const glm::vec3 t2 = (aabb_max - origin) / direction;
	const glm::vec3 tmin = glm::min(t1, t2);
	const glm::vec3 tmax = glm::max(t1, t2);
	
	return glm::min(tmax.x, glm::min(tmax.y, tmax.z)) > glm::max(glm::max(tmin.x, 0.f), glm::max(tmin.y, tmin.z));
}

export bool intersect_sphere(const glm::vec3& origin, const glm::vec3& direction, glm::vec3 position, float radius) {
	glm::vec3 op = position - origin;
	float b = glm::dot(op, direction);
	float disc = b * b - dot(op, op) + radius * radius;
	return disc >= 0.0f;
}

// Only works with uniform scaling
export void transform_aabb_uniform(const glm::vec3& min, const glm::vec3& max, glm::vec3& new_min, glm::vec3& new_max, const glm::mat4& transform) {
	new_min = transform[3];
	new_max = transform[3];
	for (size_t i = 0; i < 3; i++) {
		for (size_t j = 0; j < 3; j++) {
			float e = transform[i][j] * min[j];
			float f = transform[i][j] * max[j];
			if (e < f) {
				new_min[i] += e;
				new_max[i] += f;
			} else {
				new_min[i] += f;
				new_max[i] += e;
			}
		}
	}
}

// Works with non uniform scaling. For uniform scaling use the faster transform_aabb_uniform()
export void transform_aabb_non_uniform(const glm::vec3& min, const glm::vec3& max, glm::vec3& new_min, glm::vec3& new_max, const glm::mat4& transform) {
	glm::vec4 p1 = transform * glm::vec4(min, 1.f);
	glm::vec4 p2 = transform * glm::vec4(max.x, min.y, min.z, 1.f);
	glm::vec4 p3 = transform * glm::vec4(max.x, max.y, min.z, 1.f);
	glm::vec4 p4 = transform * glm::vec4(min.x, max.y, min.z, 1.f);
	glm::vec4 p5 = transform * glm::vec4(min.x, min.y, max.z, 1.f);
	glm::vec4 p6 = transform * glm::vec4(max.x, min.y, max.z, 1.f);
	glm::vec4 p7 = transform * glm::vec4(max, 1.f);
	glm::vec4 p8 = transform * glm::vec4(min.x, max.y, max.z, 1.f);

	new_min = glm::min(p1, glm::min(p2, glm::min(p3, glm::min(p4, glm::min(p5, glm::min(p6, glm::min(p7, p8)))))));
	new_max = glm::max(p1, glm::max(p2, glm::max(p3, glm::max(p4, glm::max(p5, glm::max(p6, glm::max(p7, p8)))))));
}