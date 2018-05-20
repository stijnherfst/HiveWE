#pragma once
#include <vector>

struct Import {
	bool custom;
	std::string path;
	fs::path file_path;
	int file_size;
	Import(bool c, std::string p, fs::path f, int s) : custom(c), path(p), file_path(f), file_size(s) {};
};


class Imports {
	int version = 1;
public:
	std::vector<Import> imports;
	std::map<std::string,std::vector<std::string>> directories;

	void load(BinaryReader &reader);
	void save();

	void load_dir_file(BinaryReader &reader);
	void save_dir_file();

	void save_imports();
	void remove_import(std::string path);

	void export_file(std::string path,std::string file);
	
	int import_size(std::string path);
};
