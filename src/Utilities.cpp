#include "Utilities.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm> 
#include <cctype>
#include <locale>
#include <QSettings>
#include <QPainter>
#include <QIcon>
#include <QOpenGLFunctions_4_5_Core>

#include "Texture.h"
#include "ResourceManager.h"

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <fmt/format.h>

QOpenGLFunctions_4_5_Core* gl;
Shapes shapes;


void Shapes::init() {
	gl->glCreateBuffers(1, &vertex_buffer);
	gl->glNamedBufferData(vertex_buffer, quad_vertices.size() * sizeof(glm::vec2), quad_vertices.data(), GL_STATIC_DRAW);

	gl->glCreateBuffers(1, &index_buffer);
	gl->glNamedBufferData(index_buffer, quad_indices.size() * sizeof(unsigned int) * 3, quad_indices.data(), GL_STATIC_DRAW);
}

std::string string_replaced(const std::string& source, const std::string& from, const std::string& to) {
	std::string new_string;
	new_string.reserve(source.length());  // avoids a few memory allocations

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

std::vector<std::string> split(const std::string& string, const char delimiter) {
	std::vector<std::string> elems;
	std::stringstream ss(string);

	std::string item;
	while (std::getline(ss, item, delimiter)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string_view> splitSV(std::string_view str, const char delimiter) {
	std::vector<std::string_view> output;
	output.reserve(str.size() / 2);

	for (auto first = str.data(), second = str.data(), last = first + str.size(); second != last && first != last; first = second + 1) {
		while (++second < last && *second != delimiter) {}
		if (first != second) {
			output.emplace_back(first, second - first);
		}
	}

	return output;
}

std::string to_lowercase_copy(const std::string_view& string) {
	std::string output(string);
	std::transform(output.begin(), output.end(), output.begin(), [](unsigned char c) { return std::tolower(c); });
	return output;
}

void to_lowercase(std::string& string) {
	std::transform(string.begin(), string.end(), string.begin(), [](unsigned char c) { return std::tolower(c); });
}


// trim from start (in place)
void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
		}));
}

// trim from end (in place)
void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

// trim from both ends (in place)
void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}

bool is_number(const std::string& s) {
	return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

GLuint compile_shader(const fs::path& vertex_shader, const fs::path& fragment_shader) {
	char buffer[512];
	GLint status;

	std::string vertex_source = read_text_file(vertex_shader.string());
	std::string fragment_source = read_text_file(fragment_shader.string());

	const GLuint vertex = gl->glCreateShader(GL_VERTEX_SHADER);
	const GLuint fragment = gl->glCreateShader(GL_FRAGMENT_SHADER);

	// Vertex Shader
	const char* source = vertex_source.c_str();
	gl->glShaderSource(vertex, 1, &source, nullptr);
	gl->glCompileShader(vertex);


	gl->glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);
	if (!status) {
		gl->glGetShaderInfoLog(vertex, 512, nullptr, buffer);
		fmt::print("{}\n{}\n", vertex_shader.string(), buffer);
		return -1;
	}

	// Fragment Shader
	source = fragment_source.c_str();
	gl->glShaderSource(fragment, 1, &source, nullptr);
	gl->glCompileShader(fragment);

	gl->glGetShaderiv(fragment, GL_COMPILE_STATUS, &status);
	if (!status) {
		gl->glGetShaderInfoLog(fragment, 512, nullptr, buffer);
		fmt::print("{}\n{}\n", fragment_shader.string(), buffer);
		return -1;
	}

	// Link
	const GLuint shader = gl->glCreateProgram();
	gl->glAttachShader(shader, vertex);
	gl->glAttachShader(shader, fragment);
	gl->glLinkProgram(shader);

	gl->glGetProgramiv(shader, GL_LINK_STATUS, &status);
	if (!status) {
		gl->glGetProgramInfoLog(shader, 512, nullptr, buffer);
		fmt::print("{}\n{}\n{}\n", vertex_shader.string(), fragment_shader.string(), buffer);
		return -1;
	}

	gl->glDeleteShader(vertex);
	gl->glDeleteShader(fragment);

	return shader;
}

std::string read_text_file(const fs::path& path) {
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

fs::path find_warcraft_directory() {
	QSettings settings;
	if (settings.contains("warcraftDirectory")) {
		return settings.value("warcraftDirectory").toString().toStdString();
	} else if (fs::exists("C:/Program Files (x86)/Warcraft III")) {
		return "C:/Program Files (x86)/Warcraft III";
	} else if (fs::exists("D:/Program Files (x86)/Warcraft III")) {
		return "D:/Program Files (x86)/Warcraft III";
	} else {
		return "";
	}
}

void load_modification_file(const std::string file_name, slk::SLK& base_data, slk::SLK& meta_slk, bool optional_ints) {
	BinaryReader reader = hierarchy.map_file_read(file_name);

	const int version = reader.read<uint32_t>();
	if (version != 1 && version != 2) {
		std::cout << "Unknown modification table version of " << version << " detected. Attempting to load, but may crash.\n";
	}

	load_modification_table(reader, base_data, meta_slk, false, optional_ints);
	load_modification_table(reader, base_data, meta_slk, true, optional_ints);
}

void load_modification_table(BinaryReader& reader, slk::SLK& slk, slk::SLK& meta_slk, const bool modification, bool optional_ints) {
	const uint32_t objects = reader.read<uint32_t>();
	for (size_t i = 0; i < objects; i++) {
		const std::string original_id = reader.read_string(4);
		const std::string modified_id = reader.read_string(4);

		if (modification) {
			slk.copy_row(original_id, modified_id, false);
		}

		const uint32_t modifications = reader.read<uint32_t>();

		for (size_t j = 0; j < modifications; j++) {
			const std::string modification_id = reader.read_string(4);
			const uint32_t type = reader.read<uint32_t>();

			std::string column_header = to_lowercase_copy(meta_slk.data("field", modification_id));
			if (optional_ints) {
				uint32_t level_variation = reader.read<uint32_t>();
				uint32_t data_pointer = reader.read<uint32_t>();
				if (data_pointer != 0) {
					column_header += char('a' + data_pointer - 1);
				}
				if (level_variation != 0) {
					column_header += std::to_string(level_variation);
				}

				// Can remove after checking whether this holds for many maps
				if (data_pointer != 0 && level_variation == 0) {
					assert(!(data_pointer != 0 && level_variation == 0));
				}
			}

			std::string data;
			switch (type) {
				case 0:
					data = std::to_string(reader.read<int>());
					break;
				case 1:
				case 2:
					data = std::to_string(reader.read<float>());
					break;
				case 3:
					data = reader.read_c_string();
					break;
				default: 
					std::cout << "Unknown data type " << type << " while loading modification table.";
			}
			reader.advance(4);

			if (column_header == "") {
				std::cout << "Unknown mod id: " << modification_id << "\n";
				continue;
			}

			if (modification) {
				slk.set_shadow_data(column_header, modified_id, data);
			} else {
				slk.set_shadow_data(column_header, original_id, data);
			}
		}
	}
}

void save_modification_file(const std::string file_name, slk::SLK& slk, slk::SLK& meta_slk, bool optional_ints) {
	BinaryWriter writer;
	writer.write<uint32_t>(mod_table_write_version);

	save_modification_table(writer, slk, meta_slk, false, optional_ints);
	save_modification_table(writer, slk, meta_slk, true, optional_ints);

	hierarchy.map_file_write(file_name, writer.buffer);
}

// The idea of SLKs and mod files is quite bad, but I can deal with them
// The way they are implemented is horrible though
void save_modification_table(BinaryWriter& writer, slk::SLK& slk, slk::SLK& meta_slk, bool custom, bool optional_ints) {
	// Create an temporary index to speed up field lookups
	absl::flat_hash_map<std::string, std::string> meta_index;
	for (const auto& [key, dontcare2] : meta_slk.row_headers) {
		std::string field = to_lowercase_copy(meta_slk.data("field", key));
		meta_index[field] = key;
	}

	BinaryWriter sub_writer;

	size_t count = 0;
	for (const auto& [id, properties] : slk.shadow_data) {
		// If we are writing custom objects then we only want rows with oldid set as the others are base rows
		if (!custom && properties.contains("oldid")) {
			continue;
		} else if (custom && !properties.contains("oldid")) {
			continue;
		}
		count++;

		if (custom) {
			sub_writer.write_string(properties.at("oldid"));
			sub_writer.write_string(id);
		} else {
			sub_writer.write_string(id);
			sub_writer.write<uint32_t>(0);
		}

		sub_writer.write<uint32_t>(properties.size() - (properties.contains("oldid") ? 1 : 0));

		const std::string base_id = custom ? properties.at("oldid") : id;
		std::string meta_id = custom ? properties.at("oldid") : id;
		if (slk.column_headers.contains("code")) {
			meta_id = slk.data("code", meta_id);
		}

		for (const auto& [property_id, value] : properties) {
			if (property_id == "oldid") {
				continue;
			}

			// Find the metadata ID for this field name since modification files are stupid
			std::string meta_data_key;

			int variation = 0;
			int data_pointer = 0;
			if (meta_index.contains(property_id)) {
				meta_data_key = meta_index.at(property_id);
			} else {
				// First strip off the variation/level
				size_t nr_position = property_id.find_first_of("0123456789");
				std::string without_numbers = property_id.substr(0, nr_position);

				if (nr_position != std::string::npos) {
					variation = std::stoi(property_id.substr(nr_position));
				}

				if (meta_index.contains(without_numbers)) {
					meta_data_key = meta_index.at(without_numbers);
				} else {
					// If it is a data field then it will contain a data_pointer/column at the end
					if (without_numbers.starts_with("data")) {
						data_pointer = without_numbers[4] - 'a' + 1;
						without_numbers = "data";
					}

					if (without_numbers == "data" || without_numbers == "unitid" || without_numbers == "cast") {
						// Unfortunately mapping a data field to a key is not easy so we have to iterate over the entire meta_slk
						for (const auto& [key, dontcare2] : meta_slk.row_headers) {
							if (meta_slk.data<int>("data", key) != data_pointer) {
								continue;
							}

							if (to_lowercase_copy(meta_slk.data("field", key)) != without_numbers) {
								continue;
							}

							std::string use_specific = meta_slk.data("usespecific", key);
							std::string not_specific = meta_slk.data("notspecific", key);

							// If we are in the exclude list
							if (not_specific.find(meta_id) != std::string::npos) {
								continue;
							}

							// If the include list is not empty and we are not inside
							if (!use_specific.empty() && use_specific.find(meta_id) == std::string::npos && use_specific.find(base_id) == std::string::npos) {
								continue;
							}

							meta_data_key = key;
							break;
						}
					}
				}

				if (meta_data_key.empty()) {
					puts("s");
				}
			}

			if (meta_data_key.empty()) {
				puts("s");
			}

			sub_writer.write_string(meta_data_key);
			
			int write_type = -1;
			const std::string type = meta_slk.data("type", meta_data_key);
			if (type == "int" 
				|| type == "bool" 
				|| type.ends_with("Flags") 
				|| type == "attackBits" 
				|| type == "channelType" 
				|| type == "deathType" 
				|| type == "defenseTypeInt" 
				|| type == "detectionType" 
				|| type == "spellDetail" 
				|| type == "teamColor" 
				|| type == "techAvail") {

				write_type = 0;
			} else if (type == "real") {
				write_type = 1;
			} else if (type == "unreal") {
				write_type = 2;
			} else { // string
				write_type = 3;
			}

			sub_writer.write<uint32_t>(write_type);

			if (optional_ints) {
				sub_writer.write<uint32_t>(variation);
				sub_writer.write<uint32_t>(data_pointer);
			}

			if (write_type == 0) {
				sub_writer.write<int>(std::stoi(value));
			} else if (write_type == 1 || write_type == 2) {
				sub_writer.write<float>(std::stof(value));
			} else {
				sub_writer.write_c_string(value);
			}

			sub_writer.write<uint32_t>(0);
		}
	}

	writer.write<uint32_t>(count);
	writer.write_vector(sub_writer.buffer);
}

QIcon ground_texture_to_icon(uint8_t* data, const int width, const int height) {
	QImage temp_image = QImage(data, width, height, QImage::Format::Format_RGBA8888);
	const int size = height / 4;

	auto pix = QPixmap::fromImage(temp_image.copy(0, 0, size, size));

	QIcon icon;
	icon.addPixmap(pix, QIcon::Normal, QIcon::Off);

	QPainter painter(&pix);
	painter.fillRect(0, 0, size, size, QColor(255, 255, 0, 64));
	painter.end();

	icon.addPixmap(pix, QIcon::Normal, QIcon::On);

	return icon;
}

QIcon texture_to_icon(fs::path path) { 
	auto tex = resource_manager.load<Texture>(path);
	QImage temp_image = QImage(tex->data.data(), tex->width, tex->height, tex->channels == 3 ? QImage::Format::Format_RGB888 : QImage::Format::Format_RGBA8888);
	auto pix = QPixmap::fromImage(temp_image);
	return QIcon(pix);
};

const glm::vec3 TRANSLATION_IDENTITY(0);
const glm::vec3 SCALE_IDENTITY(1);
const glm::quat ROTATION_IDENTITY(1, 0, 0, 0);

// Equivalent to, but much faster than
// worldMatrix = glm::translate(glm::mat4(1.f), position);
// worldMatrix = glm::translate(worldMatrix, pivot);
// worldMatrix *= glm::mat4_cast(rotation);
// worldMatrix = glm::scale(worldMatrix, scale);
// worldMatrix = glm::translate(worldMatrix, -pivot);
//#define TEST_MODE 1
void fromRotationTranslationScaleOrigin(const glm::quat& q, const glm::vec3& v, const glm::vec3& s, glm::mat4& out, const glm::vec3& pivot) {
	// ho tom bambadil
	// tom bombadillo
	// Retera was here
	// (This code is copied from the holy bible)
	float x = q.x;
	float y = q.y;
	float z = q.z;
	float w = q.w;
	float x2 = x + x;
	float y2 = y + y;
	float z2 = z + z;
	float xx = x * x2;
	float xy = x * y2;
	float xz = x * z2;
	float yy = y * y2;
	float yz = y * z2;
	float zz = z * z2;
	float wx = w * x2;
	float wy = w * y2;
	float wz = w * z2;
	float sx = s.x;
	float sy = s.y;
	float sz = s.z;
#ifdef TEST_MODE
	out[0][0] = (1 - (yy + zz)) * sx;
	out[1][0] = (xy + wz) * sx;
	out[2][0] = (xz - wy) * sx;
	out[3][0] = 0;
	out[0][1] = (xy - wz) * sy;
	out[1][1] = (1 - (xx + zz)) * sy;
	out[2][1] = (yz + wx) * sy;
	out[3][1] = 0;
	out[0][2] = (xz + wy) * sz;
	out[1][2] = (yz - wx) * sz;
	out[2][2] = (1 - (xx + yy)) * sz;
	out[3][2] = 0;
	out[0][3] = v.x + pivot.x - (out[0][0] * pivot.x + out[0][1] * pivot.y + out[0][2] * pivot.z);
	out[1][3] = v.y + pivot.y - (out[1][0] * pivot.x + out[1][1] * pivot.y + out[1][2] * pivot.z);
	out[2][3] = v.z + pivot.z - (out[2][0] * pivot.x + out[2][1] * pivot.y + out[2][2] * pivot.z);
	out[3][3] = 1;
#else
	out[0][0] = (1 - (yy + zz)) * sx;
	out[0][1] = (xy + wz) * sx;
	out[0][2] = (xz - wy) * sx;
	out[0][3] = 0;
	out[1][0] = (xy - wz) * sy;
	out[1][1] = (1 - (xx + zz)) * sy;
	out[1][2] = (yz + wx) * sy;
	out[1][3] = 0;
	out[2][0] = (xz + wy) * sz;
	out[2][1] = (yz - wx) * sz;
	out[2][2] = (1 - (xx + yy)) * sz;
	out[2][3] = 0;
	out[3][0] = v.x + pivot.x - (out[0][0] * pivot.x + out[1][0] * pivot.y + out[2][0] * pivot.z);
	out[3][1] = v.y + pivot.y - (out[0][1] * pivot.x + out[1][1] * pivot.y + out[2][1] * pivot.z);
	out[3][2] = v.z + pivot.z - (out[0][2] * pivot.x + out[1][2] * pivot.y + out[2][2] * pivot.z);
	out[3][3] = 1;
#endif
}


float hermite(float a, float aOutTan, float bInTan, float b, float t) {
	float factorTimes2 = t * t;
	float factor1 = factorTimes2 * (2 * t - 3) + 1;
	float factor2 = factorTimes2 * (t - 2) + t;
	float factor3 = factorTimes2 * (t - 1);
	float factor4 = factorTimes2 * (3 - 2 * t);
	return (a * factor1) + (aOutTan * factor2) + (bInTan * factor3) + (b * factor4);
}
float bezier(float a, float aOutTan, float bInTan, float b, float t) {
	float invt = 1 - t;
	float factorSquared = t * t;
	float inverseFactorSquared = invt * invt;
	float factor1 = inverseFactorSquared * invt;
	float factor2 = 3 * t * inverseFactorSquared;
	float factor3 = 3 * factorSquared * invt;
	float factor4 = factorSquared * t;
	return (a * factor1) + (aOutTan * factor2) + (bInTan * factor3) + (b * factor4);
}

//template <typename T>
//inline void interpolate(T& out, const T* start, const T* outTan, const T* inTan, const T* end, float t, int interpolationType) {
//	out = *start;
//}

float interpolate(const float start, const float outTan, const float inTan, const float end, float t, int interpolationType) {
	switch (interpolationType) {
		case 1: // LINEAR
			return glm::mix(start, end, t);
		case 2: // HERMITE
			return hermite(start, outTan, inTan, end, t);
		case 3: // BEZIER
			return bezier(start, outTan, inTan, end, t);
		default:
			return start;
	}
}

glm::vec3 interpolate(const glm::vec3 start, const glm::vec3 outTan, const glm::vec3 inTan, const glm::vec3 end, float t, int interpolationType) {
	switch (interpolationType) {
		glm::vec3 out;
		case 1: // LINEAR
			return glm::mix(start, end, t);
		case 2: // HERMITE
			out.x = hermite(start.x, outTan.x, inTan.x, end.x, t);
			out.y = hermite(start.y, outTan.y, inTan.y, end.y, t);
			out.z = hermite(start.z, outTan.z, inTan.z, end.z, t);
			return out;
		case 3: // BEZIER
			out.x = bezier(start.x, outTan.x, inTan.x, end.x, t);
			out.y = bezier(start.y, outTan.y, inTan.y, end.y, t);
			out.z = bezier(start.z, outTan.z, inTan.z, end.z, t);
			return out;
		default:
			return start;
	}
}
glm::quat interpolate(const glm::quat start, const glm::quat outTan, const glm::quat inTan, const glm::quat end, float t, int interpolationType) {
	switch (interpolationType) {
		case 1: // LINEAR
			return glm::slerp(start, end, t);
		case 2: // HERMITE
			// GLM uses both {x, y, z, w} and {w, x, y, z} convention, in different places, sometimes.
			// Their squad is {w, x, y, z} but we are elsewhere using {x, y, z, w}, so we will
			// continue using the copy of the Matrix Eater "ghostwolfSquad" for now.
			//out = glm::squad(*start, *outTan, *inTan, *end, t);
			return ghostwolfSquad(start, outTan, inTan, end, t);
		case 3: // BEZIER
			// GLM uses both {x, y, z, w} and {w, x, y, z} convention, in different places, sometimes.
			// Their squad is {w, x, y, z} but we are elsewhere using {x, y, z, w}, so we will
			// continue using the copy of the Matrix Eater "ghostwolfSquad" for now.
			//out = glm::squad(*start, *outTan, *inTan, *end, t);
			return ghostwolfSquad(start, outTan, inTan, end, t);
		default:
			return start;
	}
}
uint32_t interpolate(const uint32_t start, const uint32_t outTan, const uint32_t inTan, const uint32_t end, float t, int interpolationType) {
	return start;
}

glm::quat ghostwolfSquad(const glm::quat a, const glm::quat aOutTan, const glm::quat bInTan, const glm::quat b, float t) {
	glm::quat temp1;
	glm::quat temp2;
	temp1 = glm::slerp(a, b, t);
	temp2 = glm::slerp(aOutTan, bInTan, t);
	return glm::slerp(temp1, temp2, 2 * t * (1 - t));
}

glm::quat safeQuatLookAt(
	glm::vec3 const& lookFrom,
	glm::vec3 const& lookTo,
	glm::vec3 const& up,
	glm::vec3 const& alternativeUp) {
	glm::vec3 direction = lookTo - lookFrom;
	float directionLength = glm::length(direction);

	// Check if the direction is valid; Also deals with NaN
	if (!(directionLength > 0.0001))
		return glm::quat(1, 0, 0, 0); // Just return identity

	// Normalize direction
	direction /= directionLength;

	// Is the normal up (nearly) parallel to direction?
	if (glm::abs(glm::dot(direction, up)) > .9999f) {
		// Use alternative up
		return glm::quatLookAt(direction, alternativeUp);
	} else {
		return glm::quatLookAt(direction, up);
	}
}