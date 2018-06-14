#pragma once

class TriggerStrings {
public:
	std::unordered_map<std::string, std::string> strings;

	void load(BinaryReader& reader);
	void save() const;
};