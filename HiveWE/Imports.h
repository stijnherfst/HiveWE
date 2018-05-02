#pragma once
#include <vector>

struct Import {
	bool custom;
	std::string path;
	
	Import(bool c, std::string p) : custom(c), path(p) {};
};

class Imports {
	int version = 1;
public:
	std::vector<Import> imports;
	std::map<std::string,std::vector<std::string>> dirEntries;

	void load(BinaryReader &reader);
	void save();

	void loadDirectoryFile(BinaryReader &reader);
	void saveDirectoryFile();

};
