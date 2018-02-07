#pragma once

class BinaryWriter {
public:
	//HANDLE handle = nullptr;
	std::vector<uint8_t> buffer;
	
	//BinaryWriter(HANDLE handle = nullptr ) : handle(handle) {}

	template<typename T>
	void write(const T value) {
		T temp = value;
		//if (handle == nullptr) {
			buffer.resize(buffer.size() + sizeof(T));
			std::copy(reinterpret_cast<const char*>(&temp), reinterpret_cast<const char*>(&temp) + sizeof(T), buffer.end() - sizeof(T));
		//} else {
		//	SFileWriteFile(handle, &temp, sizeof(T), MPQ_COMPRESSION_ZLIB);
		//	auto error = GetLastError();
		//}
	}

	void write_string(const std::string& string) {
		//if (handle == nullptr) {
			buffer.resize(buffer.size() + string.size());
			std::copy(string.begin(), string.end(), buffer.end() - string.size());
		//} else {
		//	SFileWriteFile(handle, &string, string.size(), MPQ_COMPRESSION_ZLIB);
		//	auto error = GetLastError();
		//}
	}

	template<typename T>
	void write_vector(const std::vector<T>& vector) {
		//if (handle == nullptr) {
			buffer.resize(buffer.size() + vector.size() * sizeof(T));
			std::copy(vector.begin(), vector.end(), buffer.end() - vector.size() * sizeof(T));
		//} else {
		//	SFileWriteFile(handle, &vector, vector.size() * sizeof(T), MPQ_COMPRESSION_ZLIB);
		//	auto error = GetLastError();
		//}
	}
};