#pragma once

class BinaryReader {
public:
	std::vector<uint8_t> buffer;
	size_t position = 0;

	explicit BinaryReader(const std::vector<uint8_t>& buffer) : buffer(buffer) {}

	template<typename T>
	T read() {
		if (position + sizeof(T) > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}
		T result = *reinterpret_cast<T*>(&buffer[position]);
		position += sizeof(T);
		return result;
	}

	std::string read_string(size_t size) {
		if (position + size > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}
		std::string result = { reinterpret_cast<char*>(&buffer[position]), size };

		if (const size_t pos = result.find_first_of('\0', 0); pos != std::string::npos) {
			result.resize(pos);
		}

		position += size;
		return result;
	}

	std::string read_c_string() {
		std::string string(reinterpret_cast<char*>(buffer.data() + position));
		position += string.size();

		return string;
	}

	template<typename T>
	std::vector<T> read_vector(size_t size) {
		if (position + sizeof(T) * size > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}
		std::vector<T> result(reinterpret_cast<T*>(&buffer[position]), reinterpret_cast<T*>(&buffer[position]) + size);
		position += sizeof(T) * size;
		return result;
	}

	size_t remaining() const {
		return buffer.size() - position;
	}
};