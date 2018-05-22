#include <stdafx.h>

void Imports::load(BinaryReader& reader) {
	version = reader.read<uint32_t>();
	const int entries = reader.read<uint32_t>();

	for (int i = 0; i < entries; i++) {
		const int custom = reader.read<uint8_t>();
		std::string path = reader.read_c_string();

		if (path == "war3map.dir") {
			continue;
		}

		imports.emplace_back(custom ==  5 || custom == 13 , path, "", import_size(path));
	}
}

void Imports::save() {
	BinaryWriter writer;
	
	writer.write<uint32_t>(version);
	writer.write<uint32_t>(imports.size());
	for (auto&& i : imports) {
		writer.write<uint8_t>(i.custom ? 5 : 0);
		writer.write_string(i.path);
		writer.write('\0');
	}

	writer.write<uint8_t>(0);
	writer.write_string("war3map.dir");
	writer.write('\0');

	HANDLE handle;
	const bool success = SFileCreateFile(hierarchy.map.handle, "war3map.imp", 0, writer.buffer.size(), 0, MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, &handle);
	if (!success) {
		std::cout << GetLastError() << "\n";
	}

	SFileWriteFile(handle, writer.buffer.data(), writer.buffer.size(), MPQ_COMPRESSION_ZLIB);
	SFileFinishFile(handle);
}

void Imports::load_dir_file(BinaryReader &reader) {
	std::string last_directory = "";
	const int count = reader.read<uint32_t>();

	for (int i = 0; i < count; i++) {
		const bool directory = reader.read<uint8_t>();
		std::string name = reader.read_c_string();

		if (directory) {
			directories.emplace(name, std::vector<std::string>());
			last_directory = name;
		} else {
			// Check if the import still exists
			const auto found = std::find_if(imports.begin(), imports.end(), [&](Import ii) { return ii.path == name; });
			if (found != imports.end()) {
				directories[last_directory].push_back(name);
			}
		}
	}
}

void Imports::save_dir_file() {
	BinaryWriter writer;
	
	writer.write<uint32_t>(directories.size() + imports.size());
	for (auto&& [name, files] : directories) {
		writer.write<uint8_t>(true);
		writer.write_string(name);
		writer.write('\0');
		for (auto&& file : files) {
			writer.write<uint8_t>(false);
			writer.write_string(file);
			writer.write('\0');
		}
	}

	HANDLE handle;
	const bool success = SFileCreateFile(hierarchy.map.handle, "war3map.dir", 0, writer.buffer.size(), 0, MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, &handle);
	if (!success) {
		std::cout << GetLastError() << "\n";
	}

	SFileWriteFile(handle, writer.buffer.data(), writer.buffer.size(), MPQ_COMPRESSION_ZLIB);
	SFileFinishFile(handle);
}

void Imports::save_imports() {
	for (auto&& imp : imports) {
		SFileAddFileEx(hierarchy.map.handle, imp.file_path.c_str(), imp.path.c_str(), MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_NEXT_SAME);
	}
}

void Imports::remove_import(const fs::path& path) const {
	SFileRemoveFile(hierarchy.map.handle, path.string().c_str(), 0);
}

void Imports::import_file(const fs::path& path, const fs::path& file) const {
	SFileAddFileEx(hierarchy.map.handle, path.c_str(), file.string().c_str(), MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_NEXT_SAME);
}

void Imports::export_file(const fs::path& path, const fs::path& file) const {
	auto buffer = hierarchy.map.file_open(file).read();
	auto output = std::ofstream(path / file.filename(), std::ios::binary);
	output.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
}

int Imports::import_size(const fs::path& path) const {
	return hierarchy.map.file_open(path).size();
}
