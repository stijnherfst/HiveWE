#pragma once

// A thin wrapper around StormLib https://github.com/ladislav-zezula/StormLib
namespace mpq {
	class File {
	public:
		HANDLE handle = nullptr;
	
		File() = default;
		~File();
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


		std::vector<uint8_t> read() const;
		std::optional<std::vector<uint8_t>> read2() const;
		size_t size() const;
		void close() const;
	};

	class MPQ {
	public:
		HANDLE handle = nullptr;

		MPQ() = default;

		explicit MPQ(const fs::path& path, unsigned long flags = 0);
		~MPQ();
		MPQ(MPQ&& move) noexcept {
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

		bool open(const fs::path& path, unsigned long flags = 0);
		void close();
		bool compact();

		File file_open(const fs::path& path) const;
		void file_write(const fs::path& path, const std::vector<uint8_t>& data) const;
		void file_remove(const fs::path& path) const;
		bool file_exists(const fs::path& path) const;
		void file_add(const fs::path& path, const fs::path& new_path) const;
	};
}