#pragma once

class SLK {
public:
	std::map<std::string, size_t> header_to_column;
	std::vector<std::vector<std::string>> data;

	SLK(std::string path);

	void load(std::string path);
};