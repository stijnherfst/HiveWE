#pragma once

// A thin wrapper around StormLib https://github.com/ladislav-zezula/StormLib
namespace mpq {

	class File {
	public:
		HANDLE handle;

		std::vector<uint8_t> read();
		void close();
	};

	class MPQ {
	public:
		HANDLE handle;

		MPQ() {}
		MPQ(const std::wstring path);
		MPQ(File archive);

		void open(const std::wstring path);
		void open(File archive);
		void close();

		File file_open(const std::string path);
		bool file_exists(const std::string path);
	};

}