#include "stdafx.h"

namespace mpq {
	File::~File() {
		close();
	}

	std::vector<uint8_t> File::read() const {
		const uint32_t size = SFileGetFileSize(handle, nullptr);
		if (size == 0) {
			return {};
		}

		std::vector<uint8_t> buffer(size);
		
		#ifdef _MSC_VER
		unsigned long bytes_read;
		#else
		unsigned int bytes_read;
		#endif
		const bool success = SFileReadFile(handle, &buffer[0], size, &bytes_read, nullptr);
		if (!success) {
			std::cout << "Failed to read file: " << GetLastError() << std::endl;
		}
		return buffer;
	}

	std::optional<std::vector<uint8_t>> File::read2() const {
		const uint32_t size = SFileGetFileSize(handle, nullptr);
		if (size == 0) {
			return {};
		}

		std::vector<uint8_t> buffer(size);

		#ifdef _MSC_VER
		unsigned long bytes_read;
		#else
		unsigned int bytes_read;
		#endif
		const bool success = SFileReadFile(handle, &buffer[0], size, &bytes_read, nullptr);
		if (!success) {
			std::cout << "Failed to read file: " << GetLastError() << std::endl;
		}
		return buffer;
	}

	size_t File::size() const {
		return SFileGetFileSize(handle, 0);
	}

	void File::close() const {
		SFileCloseFile(handle);
	}

	// MPQ

	MPQ::MPQ(const fs::path& path, const unsigned long flags) {
		open(path, flags);
	}

	MPQ::MPQ(File archive, const unsigned long flags) {
		open(archive, flags);
	}

	MPQ::~MPQ() {
		close();
	}

	void MPQ::open(const fs::path& path, const unsigned long flags) {
		const bool opened = SFileOpenArchive(path.c_str(), 0, flags, &handle);
		if (!opened) {
			std::wcout << "Error opening " << path << " with error:" << GetLastError() << std::endl;
		}
	}

	void MPQ::open(File& archive, const unsigned long flags) {
		const std::vector<uint8_t> buffer = archive.read();

		// Create unique name for temp file
		local_path = QDir::tempPath().toStdString() + "/" + std::to_string(time(nullptr)) + ".mpq";
		std::ofstream output(local_path, std::ofstream::binary);
		if (!output) {
			std::cout << "Opening/Creation failed for: " << local_path << std::endl;
		}

		output.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(uint8_t));
		output.close();

		open(local_path, flags);
	}

	File MPQ::file_open(const fs::path& path) const {
		File file;
		#ifdef WIN32
		const bool opened = SFileOpenFileEx(handle, fs::weakly_canonical(path).string().c_str(), 0, &file.handle);
		#else
		const bool opened = SFileOpenFileEx(handle, path.string().c_str(), 0, &file.handle);
		#endif
		if (!opened) {
			std::cout << "Error opening file " << path << " with error: " << GetLastError() << std::endl;
		}
		return file;
	}

	void MPQ::file_remove(const fs::path& path) const {
		SFileRemoveFile(handle, path.string().c_str(), 0);
	}

	bool MPQ::file_exists(const fs::path& path) const {
	    #ifdef WIN32
		return SFileHasFile(handle, fs::weakly_canonical(path).string().c_str());
		#else
		return SFileHasFile(handle, path.string().c_str());
		#endif
	}

	void MPQ::close() {
		SFileCloseArchive(handle);
		handle = nullptr;
		fs::remove(local_path);
	}
}