#include "CASC.h"

#include "fmt/format.h"

namespace casc {
	File::~File() {
		close();
	}

	std::vector<uint8_t, default_init_allocator<uint8_t>> File::read() const {
		const uint32_t size = CascGetFileSize(handle, 0);
		std::vector<uint8_t, default_init_allocator<uint8_t>> buffer(size);

#ifdef _MSC_VER
		unsigned long bytes_read;
#else
		unsigned bytes_read;
#endif
		const bool success = CascReadFile(handle, buffer.data(), size, &bytes_read);
		if (!success) {
			fmt::print("Failed to read file: {}\n", GetLastError());
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

	void CASC::open(const fs::path& path, bool ptr) {
		const bool opened = CascOpenStorage((path / (ptr ? ":w3t" : ":w3")).c_str(), CASC_LOCALE_ALL, &handle);
		if (!opened) {
			fmt::print("Error opening {} with error: {}\n", path.string(), GetLastError());
		}
	}

	File CASC::file_open(const fs::path& path) const {
		File file;
		const bool opened = CascOpenFile(handle, path.string().c_str(), 0, CASC_OPEN_BY_NAME, &file.handle);
		if (!opened) {
			fmt::print("Error opening {} with error: {}\n", path.string(), GetLastError());
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