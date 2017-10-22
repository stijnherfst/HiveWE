#pragma once

// A thin wrapper around StormLib https://github.com/ladislav-zezula/StormLib
namespace mpq {

	class File {
	public:
		~File();
		File() {}
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

		HANDLE handle = nullptr;

		std::vector<uint8_t> read();
		void close();
	};

	class MPQ {
	public:
		HANDLE handle;

		MPQ() {}
		MPQ(const std::wstring path);
		MPQ(File archive);
		~MPQ();

		void open(const std::wstring path);
		void open(File archive);
		void close();

		File file_open(const std::string path);
		bool file_exists(const std::string path);
	};

}