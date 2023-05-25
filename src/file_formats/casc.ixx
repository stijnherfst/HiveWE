module;

#include <filesystem>
#include <vector>
#include <span>
#include <print>

#define __CASCLIB_SELF__
#define WIN32_LEAN_AND_MEAN
#include <CascLib.h>

export module CASC;

import no_init_allocator;

namespace fs = std::filesystem;

// A thin wrapper around CascLib https://github.com/ladislav-zezula/CascLib
namespace casc {
	export class File {
	  public:
		HANDLE handle = nullptr;

		File() = default;
		~File() {
			close();
		}
		File(File&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
		}
		File(const File&) = default;
		File& operator=(const File&) = default;
		File& operator=(File&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
			return *this;
		}

		// std::span<uint8_t> read() const;
		std::vector<uint8_t, default_init_allocator<uint8_t>> read() const {
			const uint32_t size = CascGetFileSize(handle, 0);
			std::vector<uint8_t, default_init_allocator<uint8_t>> buffer(size);

#ifdef _MSC_VER
			unsigned long bytes_read;
#else
			unsigned bytes_read;
#endif
			const bool success = CascReadFile(handle, buffer.data(), size, &bytes_read);
			if (!success) {
				std::print("Failed to read file: {}\n", GetCascError());
			}
			return buffer;
		}
		// std::pair<std::unique_ptr<uint8_t[]>, std::size_t> read() const;

		size_t size() const noexcept {
			return CascGetFileSize(handle, 0);
		}

		void close() const noexcept {
			CascCloseFile(handle);
		}
	};

	export class CASC {
	  public:
		HANDLE handle = nullptr;

		CASC() = default;

		explicit CASC(const fs::path& path) {
			open(path);
		}

		~CASC() {
			close();
		}

		CASC(CASC&& move)
		noexcept {
			handle = move.handle;
			move.handle = nullptr;
		}
		CASC(const CASC&) = default;
		CASC& operator=(const CASC&) = delete;
		CASC& operator=(CASC&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
			return *this;
		}

		bool open(const fs::path& path) {
			if (handle != nullptr)
				close();
			const bool opened = CascOpenStorage(path.c_str(), CASC_LOCALE_ALL, &handle);
			if (!opened) {
				std::print("Error opening {} with error: {}\n", path.string(), GetCascError());
			}
			return opened;
		}

		void close() {
			CascCloseStorage(handle);
			handle = nullptr;
		}

		File file_open(const fs::path& path) const {
			File file;
			const bool opened = CascOpenFile(handle, path.string().c_str(), 0, CASC_OPEN_BY_NAME, &file.handle);
			if (!opened) {
				std::print("Error opening {} with error: {}\n", path.string(), GetCascError());
			}
			return file;
		}

		/// ToDo is there a better way to check if a file exists?
		bool file_exists(const fs::path& path) const {
			File file;
			return CascOpenFile(handle, path.string().c_str(), 0, CASC_OPEN_BY_NAME, &file.handle);
		}
	};
} // namespace casc