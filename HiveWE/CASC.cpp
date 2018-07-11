#include "stdafx.h"

namespace casc {
	File::~File() {
		close();
	}

	std::vector<uint8_t> File::read() const {
		const uint32_t size = CascGetFileSize(handle, 0);
		std::vector<uint8_t> buffer(size);

		unsigned long bytes_read;
		const bool success = CascReadFile(handle, &buffer[0], size, &bytes_read);
		if (!success) {
			std::cout << "Failed to read file: " << GetLastError() << std::endl;
		}
		return buffer;
	}

	size_t File::size() const {
		return CascGetFileSize(handle, 0);
	}

	void File::close() const {
		CascCloseFile(handle);
	}

	// CASC

	CASC::CASC(const fs::path& path, const unsigned long flags) {
		open(path, flags);
	}

	//CASC::CASC(File archive, const unsigned long flags) {
	//	open(archive, flags);
	//}

	CASC::~CASC() {
		close();
	}

	void CASC::open(const fs::path& path, const unsigned long flags) {
		const bool opened = CascOpenStorage(path.c_str(), CASC_LOCALE_ALL, &handle);
		if (!opened) {
			std::wcout << "Error opening " << path << " with error:" << GetLastError() << std::endl;
		}
	}

	//void CASC::open(File& archive, const unsigned long flags) {
	//	const std::vector<uint8_t> buffer = archive.read();

	//	// Create unique name for local file
	//	local_path = "Data/Temporary/" + std::to_string(time(nullptr)) + ".mpq";
	//	std::ofstream output(local_path, std::ofstream::binary);
	//	if (!output) {
	//		std::cout << "Opening/Creation failed for: " << local_path << std::endl;
	//	}

	//	output.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(uint8_t));
	//	output.close();

	//	open(local_path, flags);
	//}

	File CASC::file_open(const fs::path& path) const {
		File file;
		const bool opened = CascOpenFile(handle, fs::weakly_canonical(path).string().c_str(), 0, 0, &file.handle);
		if (!opened) {
			std::cout << "Error opening file " << path << " with error: " << GetLastError() << std::endl;
		}
		return file;
	}

	//void CASC::file_remove(const fs::path& path) const {
	//	SFileRemoveFile(handle, path.string().c_str(), 0);

	//}

	/// ToDo is there a better way to check if a file exists?
	bool CASC::file_exists(const fs::path& path) const {
		File file;
		const bool exists = CascOpenFile(handle, fs::weakly_canonical(path).string().c_str(), 0, 0, &file.handle);
		CascCloseFile(file.handle);
		return exists;
	}

	void CASC::close() {
		CascCloseStorage(handle);
		handle = nullptr;
		fs::remove(local_path);
	}
}