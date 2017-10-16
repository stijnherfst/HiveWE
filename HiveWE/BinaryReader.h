#pragma once

class BinaryReader {
public:
	std::vector<uint8_t> buffer;
	size_t position = 0;

	BinaryReader(std::vector<uint8_t> buffer) : buffer(buffer) {}

	template<typename T>
	T read() {
		T result = *reinterpret_cast<T*>(&buffer[position]);
		position += sizeof(T);
		return result;
	}

	std::string readString(size_t size) {
		std::string result = { reinterpret_cast<char*>(&buffer[position]), size };
		position += size;
		return result;
	}

	template<typename T>
	std::vector<T> readVector(size_t size) {
		std::vector<T> result(reinterpret_cast<T*>(&buffer[position]), reinterpret_cast<T*>(&buffer[position]) + size);
		position += sizeof(T) * size;
		return result;
	}
};