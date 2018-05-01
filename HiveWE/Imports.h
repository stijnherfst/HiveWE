#pragma once
#include <vector>

struct Import {
	bool isCustom;
	std::string path;
	
	Import(bool c,std::string p) {
		isCustom = c;
		path = p;
	}
};

class Imports {
	int version = 1;
public:
	std::vector<Import> imports;
	std::vector<std::string> dirEntries;

	void load(BinaryReader &reader);
	void save();

	void loadDirectoryFile(BinaryReader &reader);
	void saveDirectoryFile();

};
