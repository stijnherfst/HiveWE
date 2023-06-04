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
	std::vector<std::pair<std::string, int>> items;
};


export inline int16_t floatToSnorm16_64(float v) {
	// According to D3D10 rules, the value "-1.0f" has two representations: 0x1000 and 0x10001
	// This allows everyone to convert by just multiplying by 32767 instead
	// of multiplying the negative values by 32768 and 32767 for positive.
	//std::println("{}", static_cast<int16_t>(std::clamp(v >= 0.0f ? (v * 32767.0f + 0.5f) : (v * 32767.0f - 0.5f), -32768.0f, 32767.0f)));
	return static_cast<int16_t>(std::clamp(v >= 0.0f ? (v * (32767.0f / 64.f) + 0.5f) : (v * (32767.0f / 64.f) - 0.5f), (32768.0f / 64.f), (32767.0f / 64.f)));
}

export inline int16_t floatToSnorm16(float v) {
	// According to D3D10 rules, the value "-1.0f" has two representations: 0x1000 and 0x10001
	// This allows everyone to convert by just multiplying by 32767 instead
	// of multiplying the negative values by 32768 and 32767 for positive.
	//std::println("{}", static_cast<int16_t>(std::clamp(v >= 0.0f ? (v * 32767.0f + 0.5f) : (v * 32767.0f - 0.5f), -32768.0f, 32767.0f)));
	return static_cast<int16_t>(std::clamp(v >= 0.0f ? (v * 32767.0f + 0.5f) : (v * 32767.0f - 0.5f), -32768.0f, 32767.0f));
}