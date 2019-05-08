#include "stdafx.h"

namespace casc {
	File::~File() {
		close();
	}

	std::vector<uint8_t> File::read() const {
		const uint32_t size = CascGetFileSize(handle, 0);
		std::vector<uint8_t> buffer(size);

		#ifdef _MSC_VER
		unsigned long bytes_read;
		#else
		unsigned bytes_read;
		#endif
		const bool success = CascReadFile(handle, buffer.data(), size, &bytes_read);
		if (!success) {
			std::cout << "Failed to read file: " << GetLastError() << std::endl;
		}
		return buffer;
	}

	size_t File::size() const noexcept {
		return CascGetFileSize(handle, 0);
	}

	void File::close() const noexcept {
		CascCloseFile(handle);
	}

	// CASC
	CASC::CASC(const fs::path& path) {
		open(path);
	}

	CASC::~CASC() {
		close();
	}

	void CASC::open(const fs::path& path) {
		const bool opened = CascOpenStorage(path.c_str(), CASC_LOCALE_ALL, &handle);
		if (!opened) {
			std::wcout << "Error opening " << path << " with error:" << GetLastError() << std::endl;
		}
	}

	File CASC::file_open(const fs::path& path) const {
		File file;
		const bool opened = CascOpenFile(handle, path.string().c_str(), 0, CASC_OPEN_BY_NAME, &file.handle);
		if (!opened) {
			std::cout << "Error opening file " << path << " with error: " << GetLastError() << std::endl;
		}
		return file;
	}

	/// ToDo is there a better way to check if a file exists?
	bool CASC::file_exists(const fs::path& path) const {
		File file;
		return CascOpenFile(handle, path.string().c_str(), 0, CASC_OPEN_BY_NAME, &file.handle);
	}

	void CASC::close() {
		CascCloseStorage(handle);
		handle = nullptr;
	}
}