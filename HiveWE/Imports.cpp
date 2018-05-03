#include <stdafx.h>

void Imports::load(BinaryReader& reader) {
	version = reader.read<uint32_t>();
	int entries = reader.read<uint32_t>();

	for (int i = 0; i < entries; i++) {
		int is_custom = reader.read<uint8_t>();
		std::string path = reader.read_c_string();
		reader.advance(1);
		imports.emplace_back(is_custom ==  5 || is_custom == 13 , path,"");
	}

}

void Imports::save() {
	BinaryWriter writer;
	// Remove the item if it exists.
	imports.erase(std::remove_if(imports.begin(), imports.end(), [&](const Import &imp) { return imp.path == "war3map.dir"; }), imports.end());
	// Re add the item to the end of the vector.
	imports.emplace_back(false, "war3map.dir","");
	
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
	std::cout << ".imp file is saved." << std::endl;
}

void Imports::loadDirectoryFile(BinaryReader &reader) {
	int count = reader.read<uint32_t>();
	
	for (int i = 0; i < count; i++) {
		std::string name = reader.read_c_string();
		reader.advance(1);

		int nof = 0;
		std::string last_dir;

		if ( split(name, '.').back() == "dir" ) {
			std::cout << "Entering directory: " << name << std::endl;
			dirEntries.emplace(name, std::vector<std::string>());
			nof = reader.read<uint32_t>();
			last_dir.swap(name);

			for (int j = 0; j < nof; j++) {
				name = reader.read_c_string();
				reader.advance(1);
				std::cout << "\t- " << name << std::endl;
				dirEntries.at(last_dir).push_back(name);
			}
		}
	}

}


void Imports::saveDirectoryFile() {
	BinaryWriter writer;
	
	writer.write<uint32_t>(dirEntries.size());
	for (auto&&[name, files] : dirEntries) {
		std::cout << "Saving directory: " << name << std::endl;
		writer.write_string(name);
		writer.write('\0');
		writer.write<uint32_t>(files.size());
		for (auto&& f: files) {
			std::cout << "\t- " << f << std::endl;
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