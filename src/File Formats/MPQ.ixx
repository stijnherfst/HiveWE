module;

#include <string>
#include <vector>
#include <stdexcept>
#include <optional>
#include <iostream>
#include <filesystem>

#define STORMLIB_NO_AUTO_LINK
#include <StormLib.h>

export module MPQ;

namespace fs = std::filesystem;

// A thin wrapper around StormLib https://github.com/ladislav-zezula/StormLib
namespace mpq {
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

		std::vector<uint8_t> read() const {
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
		std::optional<std::vector<uint8_t>> read2() const {
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

		size_t size() const {
			return SFileGetFileSize(handle, nullptr);
		}

		void close() const {
			SFileCloseFile(handle);
		}
	};

	export class MPQ {
	  public:
		HANDLE handle = nullptr;

		MPQ() = default;

		explicit MPQ(const fs::path& path, const unsigned long flags = 0) {
			open(path, flags);
		}

		~MPQ() {
			close();
		}
		MPQ(MPQ&& move)
		noexcept {
			handle = move.handle;
			move.handle = nullptr;
		}
		MPQ(const MPQ&) = default;
		MPQ& operator=(const MPQ&) = delete;
		MPQ& operator=(MPQ&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
			return *this;
		}

		bool open(const fs::path& path, const unsigned long flags = 0) {
			return SFileOpenArchive(path.c_str(), 0, flags, &handle);
		}

		void close() {
			SFileCloseArchive(handle);
			handle = nullptr;
		}

		bool compact() {
			return SFileCompactArchive(handle, nullptr, false);
		}

		bool unpack(const fs::path& path) {
			SFILE_FIND_DATA file_data;
			HANDLE find_handle = SFileFindFirstFile(handle, "*", &file_data, nullptr);
			fs::create_directories((path / file_data.cFileName).parent_path());
			SFileExtractFile(handle, file_data.cFileName, (path / file_data.cFileName).c_str(), SFILE_OPEN_FROM_MPQ);

			while (SFileFindNextFile(find_handle, &file_data)) {
				fs::create_directories((path / file_data.cFileName).parent_path());
				SFileExtractFile(handle, file_data.cFileName, (path / file_data.cFileName).c_str(), SFILE_OPEN_FROM_MPQ);
			}
			SFileFindClose(find_handle);

			// Delete unneeded files
			fs::remove(path / "(listfile)");
			fs::remove(path / "(attributes)");
			fs::remove(path / "(war3map.imp)");
			return true;
		}

		File file_open(const fs::path& path) const {
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

		void file_write(const fs::path& path, const std::vector<uint8_t>& data) const {
			HANDLE out_handle;
			bool success = SFileCreateFile(handle, path.string().c_str(), 0, static_cast<DWORD>(data.size()), 0, MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, &out_handle);
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

		void file_remove(const fs::path& path) const {
			SFileRemoveFile(handle, path.string().c_str(), 0);
		}

		bool file_exists(const fs::path& path) const {
#ifdef WIN32
			return SFileHasFile(handle, fs::weakly_canonical(path).string().c_str());
#else
			return SFileHasFile(handle, path.string().c_str());
#endif
		}

		void file_add(const fs::path& path, const fs::path& new_path) const {
#ifdef _MSC_VER
			bool success = SFileAddFileEx(handle, path.wstring().c_str(), new_path.string().c_str(), MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_ZLIB);
#else
			bool success = SFileAddFileEx(handle, path.string().c_str(), new_path.string().c_str(), MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_ZLIB);
#endif
			if (!success) {
				std::cout << "Error adding file: " << GetLastError() << "\n";
			}
		}
	};
} // namespace mpq