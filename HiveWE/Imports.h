#pragma once
#include <vector>

struct Import {
	bool custom;
	std::string path;
	fs::path file_path;
	int size;

	Import(const bool custom, const std::string path, const fs::path file_path, const int size) : custom(custom), path(path), file_path(file_path), size(size) {}
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
	void remove_import(const fs::path& path) const;

	void import_file(const fs::path& path, const fs::path& file) const;
	void export_file(const fs::path& path, const fs::path& file) const;
	
	int import_size(const fs::path& path) const;
};
