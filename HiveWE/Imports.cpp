#include <stdafx.h>

void Imports::load(BinaryReader& reader) {
	version = reader.read<uint32_t>();
	int entries = reader.read<uint32_t>();

	for (int i = 0; i < entries; i++) {
		int is_custom = reader.read<uint8_t>();
		std::string path = reader.read_c_string();
		reader.advance(1);
		imports.emplace_back(is_custom ==  5 || is_custom == 13 , path,"", import_size(path));
	}

}

void Imports::save() {
	BinaryWriter writer;
	// Remove the item if it exists.
	imports.erase(std::remove_if(imports.begin(), imports.end(), [&](const Import &imp) { return imp.path == "war3map.dir"; }), imports.end());
	// Re add the item to the end of the vector.
	imports.emplace_back(false, "war3map.dir","",0);
	
	writer.write<uint32_t>(version);
	writer.write<uint32_t>(imports.size());
	for (auto&& i : imports) {
		writer.write<uint8_t>(i.custom ? 5: 0);
		writer.write_string(i.path);
		writer.write('\0');
	}

	HANDLE handle;
	const bool success = SFileCreateFile(hierarchy.map.handle, "war3map.imp", 0, writer.buffer.size(), 0, MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, &handle);
	if (!success) {
		std::cout << GetLastError() << "\n";
	}

	SFileWriteFile(handle, writer.buffer.data(), writer.buffer.size(), MPQ_COMPRESSION_ZLIB);
	SFileFinishFile(handle);
}

void Imports::load_dir_file(BinaryReader &reader) {
	int count = reader.read<uint32_t>();
	
	for (int i = 0; i < count; i++) {
		std::string name = reader.read_c_string();
		reader.advance(1);

		int nof = 0;
		std::string last_dir;

		if ( split(name, '.').back() == "dir" ) {
			directories.emplace(name, std::vector<std::string>());
			nof = reader.read<uint32_t>();
			last_dir.swap(name);

			for (int j = 0; j < nof; j++) {
				name = reader.read_c_string();
				reader.advance(1);
				directories.at(last_dir).push_back(name);
			}
		}
	}

}


void Imports::save_dir_file() {
	BinaryWriter writer;
	
	writer.write<uint32_t>(directories.size());
	for (auto&&[name, files] : directories) {
		writer.write_string(name);
		writer.write('\0');
		writer.write<uint32_t>(files.size());
		for (auto&& f: files) {
			writer.write_string(f);
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


void Imports::remove_import(std::string path) {
	SFileRemoveFile(hierarchy.map.handle, path.c_str(), 0);
}


void Imports::export_file(std::string path,std::string file) {
	HANDLE handle;
	std::string s = fs::path(file).filename().string();
	std::ofstream out_file(path + "\\" + s, std::ios::binary | std::ios::out | std::ios::ate);
	const bool success = SFileOpenFileEx(hierarchy.map.handle, file.c_str(), SFILE_OPEN_FROM_MPQ, &handle);
	if (!success) {
		std::cout << GetLastError() << "\n";
	}
	auto size = SFileGetFileSize(handle, 0);
	std::vector<char> buffer(size);
	SFileReadFile(handle, buffer.data(), size, 0, 0);
	out_file.write(buffer.data(), size);

	out_file.close();
	SFileCloseFile(handle);
}

int Imports::import_size(std::string path) {
	HANDLE handle;
	const bool success = SFileOpenFileEx(hierarchy.map.handle, path.c_str(), SFILE_OPEN_FROM_MPQ, &handle);
	if (!success) {
		std::cout << GetLastError() << "\n";
	}
	int size = SFileGetFileSize(handle, 0);

	SFileCloseFile(handle);

	return size;
}