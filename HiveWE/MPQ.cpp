#include "stdafx.h"

namespace mpq {

	std::vector<uint8_t> File::read() {
		uint32_t size = SFileGetFileSize(handle, nullptr);
		std::vector<uint8_t> buffer(size);

		unsigned long dwBytes;
		bool success = SFileReadFile(handle, &buffer[0], size, &dwBytes, NULL);
		if (!success) {
			std::cout << "Failed to read file: " << GetLastError() << std::endl;
		}
		return buffer;
	}

	void File::close() {
		SFileCloseFile(handle);
	}

	File::~File() {
		close();
	}

	// MPQ

	MPQ::MPQ(const std::wstring path) {
		open(path);
	}

	MPQ::MPQ(File archive) {
		open(archive);
	}

	MPQ::~MPQ() {
		SFileCloseArchive(handle);
	}

	void MPQ::open(const std::wstring path) {
		bool opened = SFileOpenArchive(path.c_str(), 0, STREAM_FLAG_READ_ONLY, &handle);
		if (!opened) {
			std::wcout << "Error opening " << path << " with error:" << GetLastError() << std::endl;
		}
	}
	
	void MPQ::open(File archive) {
		const std::vector<uint8_t> buffer = archive.read();

		std::ofstream output("Data/Temporary/temp.mpq", std::ofstream::binary);
		if (!output) {
			std::cout << "Opening/Creating failed for-: Data/Temporary/temp.mpq" << std::endl;
		}

		output.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(uint8_t));
		output.close();

		open(L"Data/Temporary/temp.mpq");
	}

	File MPQ::file_open(const std::string path) {
		File file;
		bool opened = SFileOpenFileEx(handle, path.c_str(), 0, &file.handle);
		if (!opened) {
			std::cout << "Error opening file " << path << " with error: " << GetLastError() << std::endl;
		}
		return file;
	}

	bool MPQ::file_exists(const std::string path) {
		return SFileHasFile(handle, path.c_str());
	}

	void MPQ::close() {
		SFileCloseArchive(handle);
	}
}