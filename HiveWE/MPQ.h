#pragma once

// A thin wrapper around StormLib https://github.com/ladislav-zezula/StormLib
namespace mpq {
	class File {
	public:
		HANDLE handle = nullptr;
		
		File() {}
		~File();
		File(File&& move) {
			handle = move.handle;
			move.handle = nullptr;
		}
		File(const File&) = default;
		File& operator=(const File&) = default;
		File& operator=(File&& move) {
			handle = move.handle;
			move.handle = nullptr;
			return *this;
		}


		std::vector<uint8_t> read();
		void close();
	};

	class MPQ {
	public:
		HANDLE handle;

		MPQ() {}
		MPQ(const std::wstring path, unsigned long flags = 0);
		MPQ(File archive, unsigned long flags = 0);
		~MPQ();
		MPQ(MPQ&& move) {
			handle = move.handle;
			move.handle = nullptr;
		}
		MPQ(const MPQ&) = default;
		MPQ& operator=(const MPQ&) = delete;
		MPQ& operator=(MPQ&& move) {
			handle = move.handle;
			move.handle = nullptr;
			return *this;
		}

		void open(const std::wstring path, unsigned long flags = 0);
		void open(File& archive, unsigned long flags = 0);
		void close();

		File file_open(const std::string path);
		bool file_exists(const fs::path path);
	};

}