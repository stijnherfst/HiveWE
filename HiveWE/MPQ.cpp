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
		const bool success = SFileReadFile(handle, buffer.data(), size, &bytes_read, nullptr);
		if (!success) {
			throw std::runtime_error("Failed to read file: " + std::to_string(GetLastError()));
		}
		return buffer;
	}

	/// An implementation using optional. Use this for all reads?
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
		const bool success = SFileReadFile(handle, buffer.data(), size, &bytes_read, nullptr);
		if (!success) {
			std::cout << "Failed to read file: " << GetLastError() << std::endl;
		}
		return buffer;
	}
	
	size_t File::size() const {
		return SFileGetFileSize(handle, nullptr);
	}

	void File::close() const {
		SFileCloseFile(handle);
	}

	// MPQ

	MPQ::MPQ(const fs::path& path, const unsigned long flags) {
		open(path, flags);
	}

	MPQ::~MPQ() {
		close();
	}

	bool MPQ::open(const fs::path& path, const unsigned long flags) {
		return SFileOpenArchive(path.c_str(), 0, flags, &handle);
	}

	File MPQ::file_open(const fs::path& path) const {
		File file;
		#ifdef WIN32
		const bool opened = SFileOpenFileEx(handle, fs::weakly_canonical(path).string().c_str(), 0, &file.handle);
		#else
		const bool opened = SFileOpenFileEx(handle, path.string().c_str(), 0, &file.handle);
		#endif
		if (!opened) {
			throw std::runtime_error("Failed to read file " + path.string() + " with error: " + std::to_string(GetLastError()));
		}
		return file;
	}
	
	void MPQ::file_write(const fs::path& path, const std::vector<uint8_t>& data) const {
		HANDLE out_handle;
		bool success = SFileCreateFile(handle , path.string().c_str(), 0, static_cast<DWORD>(data.size()), 0, MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, &out_handle);
		if (!success) {
			std::cout << GetLastError() << " " << path << "\n";
		}

		success = SFileWriteFile(out_handle, data.data(), static_cast<DWORD>(data.size()), MPQ_COMPRESSION_ZLIB);
		if (!success) {
			std::cout << "Writing to file failed: " << GetLastError() << " " << path << "\n";
		}

		success = SFileFinishFile(out_handle);
		if (!success) {
			std::cout << "Finishing write failed: " << GetLastError() << " " << path << "\n";
		}
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

	void MPQ::file_add(const fs::path& path, const fs::path& new_path) const {
		bool success = SFileAddFileEx(handle, path.wstring().c_str(), new_path.string().c_str(), MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_ZLIB);
		if (!success) {
			std::cout << "Error adding file: " << GetLastError() << "\n";
		}
	}


	void MPQ::close() {
		SFileCloseArchive(handle);
		handle = nullptr;
	}

	bool MPQ::compact() {
		return SFileCompactArchive(handle, nullptr, false);
	}
}