module;

#include <stdexcept>
#include <vector>
#include <string>

export module BinaryReader;

import no_init_allocator;

export class BinaryReader {
  public:
	std::vector<uint8_t, default_init_allocator<uint8_t>> buffer;
	unsigned long long int position = 0;

	explicit BinaryReader(std::vector<uint8_t, default_init_allocator<uint8_t>> buffer)
		: buffer(std::move(buffer)) {
	}

	template <typename T>
	[[nodiscard]] T read() {
		static_assert(std::is_standard_layout<T>::value, "T must be of standard layout.");

		if (position + sizeof(T) > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}
		T result = *reinterpret_cast<T*>(&buffer[position]);

		position += sizeof(T);
		return result;
	}

	[[nodiscard]] std::string read_string(const size_t size) {
		if (position + size > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}
		std::string result = { reinterpret_cast<char*>(&buffer[position]), static_cast<size_t>(size) };

		if (const size_t pos = result.find_first_of('\0', 0); pos != std::string::npos) {
			result.resize(pos);
		}

		position += size;
		return result;
	}

	[[nodiscard]] std::string read_c_string() {
		const std::string string(reinterpret_cast<char*>(buffer.data() + position));
		position += string.size() + 1;

		if (position > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}

		return string;
	}

	template <typename T>
	[[nodiscard]] std::vector<T> read_vector(const size_t size) {
		static_assert(std::is_standard_layout<T>::value, "T must be of standard layout.");

		if (position + sizeof(T) * size > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}
		std::vector<T> result(reinterpret_cast<T*>(&buffer[position]), reinterpret_cast<T*>(&buffer[position]) + size);
		position += sizeof(T) * size;
		return result;
	}

	[[nodiscard]] long long remaining() const {
		return buffer.size() - position;
	}

	void advance(const size_t amount) {
		if (position + amount > buffer.size()) {
			throw std::out_of_range("Trying to advance past the end of the buffer");
		}
		position += amount;
	}

	void advance_c_string() {
		position += std::string(reinterpret_cast<char*>(buffer.data() + position)).size() + 1;

		if (position > buffer.size()) {
			throw std::out_of_range("Trying to read out of range of buffer");
		}
	}
};