#include <stdafx.h>

void Imports::load(BinaryReader& reader) {
	int version = reader.read<uint32_t>(); // ToDo check version
	std::cout << "Import version: " << version << "\n";
	const int entries = reader.read<uint32_t>();
	for (int i = 0; i < entries; i++) {
		const int custom = reader.read<uint8_t>();
		QString path = QString::fromStdString(reader.read_c_string());
		// Strip any war3mapImported
		path.remove("war3mapImported\\");
		path.remove("war3mapImported/");

		if (path == "war3map.dir") {
			continue;
		}

		ImportItem item;
		item.custom = custom == 10 || custom == 13;
		item.name = path.toStdString();
		item.full_path = (item.custom ? "" : "war3mapImported\\") + path.toStdString();
		item.size = file_size(item.full_path);
		uncategorized.push_back(item);
	}
}

void Imports::save() const {
	BinaryWriter writer;
	
	writer.write<uint32_t>(1);

	int item_count = 0;
	std::function<void(const std::vector<ImportItem>&)> count_files = [&](const std::vector<ImportItem>& items) {
		for (const auto& i : items) {
			item_count++;
			count_files(i.children);
		}
	};
	count_files(imports);

	writer.write<uint32_t>(item_count);
	std::function<void(const std::vector<ImportItem>&)> save_directories = [&](const std::vector<ImportItem>& items) {
		for (const auto& i : items) {
			if (i.directory) {
				save_directories(i.children);
			} else {
				writer.write<uint8_t>(i.custom ? 13 : 8);
				writer.write_c_string(i.name.string());
			}
		}
	};

	save_directories(imports);

	writer.write<uint8_t>(0);
	writer.write_c_string("war3map.dir");


	hierarchy.map_file_write("war3map.imp", writer.buffer);
}

void Imports::load_dir_file(BinaryReader& reader) {
	std::string last_directory;

	const int version = reader.read<uint32_t>();
	if (version != 1) {
		std::cout << "Attempting to read a newer directory file version:" << version << ". Program may crash";
	}

	const std::function<void(std::vector<ImportItem>&)> read_directory = [&](std::vector<ImportItem>& items) {
		const int count = reader.read<uint32_t>();
		for (int i = 0; i < count; i++) {
			ImportItem item;
			item.directory = reader.read<uint8_t>();
			item.name = reader.read_c_string();

			item.custom = reader.read<uint8_t>();

			if (item.directory) {
				read_directory(item.children);
			} else {
				item.full_path = (item.custom ? ""s : "war3mapImported\\"s) + item.name.string();

				item.size = file_size(item.full_path);

				auto found = std::find_if(uncategorized.begin(), uncategorized.end(), [&](ImportItem x) { return x.name == item.name; });
				if (found != uncategorized.end()) {
					uncategorized.erase(found);
				}
			}

			items.push_back(item);
		}
	};

	read_directory(imports);
}

void Imports::save_dir_file() const {
	BinaryWriter writer;
	
	// Version
	writer.write<uint32_t>(1);

	std::function<void(const std::vector<ImportItem>&)> save_directories = [&](const std::vector<ImportItem>& items) {
		writer.write<uint32_t>(items.size());
		for (auto&& i : items) {
			writer.write<uint8_t>(i.directory);
			writer.write_c_string(i.name.string());
			writer.write<uint8_t>(i.custom);

			if (i.directory) {
				save_directories(i.children);
			}
		}
	};

	save_directories(imports);

	hierarchy.map_file_write("war3map.dir", writer.buffer);
}

void Imports::populate_uncategorized() {
	for (const auto& i : uncategorized) {
		imports.push_back(i);
	}
}

void Imports::remove_file(const fs::path& file) const {
	//SFileRemoveFile(hierarchy.map.handle, file.string().c_str(), 0);
	hierarchy.map_file_remove(file);
}

bool Imports::import_file(const fs::path& path, const fs::path& file) const {
	//return SFileAddFileEx(hierarchy.map.handle, path.c_str(), file.string().c_str(), MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_NEXT_SAME);
//	hierarchy.map.file_add(path, file);
	return false;
}

void Imports::export_file(const fs::path& path, const fs::path& file) const {
	// ToDo use common folder interface
//	auto buffer = hierarchy.map.file_open(file).read();
	//auto output = std::ofstream(path / file.filename(), std::ios::binary);
	//output.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
}

size_t Imports::file_size(const fs::path& file) const {
//	return hierarchy.map.file_open(file).size();
	return 0;
}

std::vector<std::reference_wrapper<const ImportItem>> Imports::find(std::function<bool(const ImportItem&)> predicate) const {
	std::vector<std::reference_wrapper<const ImportItem>> found_items;

	std::function<void(const std::vector<ImportItem>&)> find_items = [&](const std::vector<ImportItem>& items) {
		for (auto&& i : items) {
			if (i.directory) {
				find_items(i.children);
			} else {
				if (predicate(i)) {
					found_items.emplace_back(i);
				}
			}
		}
	};

	find_items(imports);

	return found_items;
}