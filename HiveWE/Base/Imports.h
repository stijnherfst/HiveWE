#pragma once
#include <vector>

struct ImportItem {
	bool directory = false;
	bool custom = false;
	fs::path name;
	int size;
	fs::path full_path;

	std::vector<ImportItem> children;
};

class Imports {
public:
	//std::vector<ImportItem> war3map_imp;
	std::vector<ImportItem> uncategorized;
	std::vector<ImportItem> imports;

	void load(BinaryReader &reader);
	void save() const;

	void load_dir_file(BinaryReader &reader);
	void save_dir_file() const;

	void populate_uncategorized();

	void remove_file(const fs::path& file) const;
	bool import_file(const fs::path& path, const fs::path& file) const;
	void export_file(const fs::path& path, const fs::path& file) const;
	size_t file_size(const fs::path& file) const;

	/// Returns a flat list of references to ImportItems
	std::vector<std::reference_wrapper<const ImportItem>> find(std::function<bool(const ImportItem&)> predicate) const;
};