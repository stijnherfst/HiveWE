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
		void close() const;
	};

	class MPQ {
	public:
		HANDLE handle;
		fs::path local_path;

		MPQ() = default;

		explicit MPQ(const fs::path& path, unsigned long flags = 0);
		explicit MPQ(File archive, unsigned long flags = 0);
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

		void open(const fs::path& path, unsigned long flags = 0);
		void open(File& archive, unsigned long flags = 0);
		void close();

		File file_open(const fs::path& path) const;
		bool file_exists(const fs::path& path) const;
	};

}