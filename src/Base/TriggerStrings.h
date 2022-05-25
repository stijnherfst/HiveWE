#pragma once

import BinaryReader;

#include <map>
#include<string>

class TriggerStrings {
	std::map<std::string, std::string> strings; // ToDo change back to unordered_map?

	size_t next_id;
public:
	void load();
	void save() const;

	std::string string(const std::string& key) const;
	void set_string(std::string& key, const std::string& value);
};