#pragma once

class BinaryWriter {
public:
	std::vector<uint8_t> buffer;

	template<typename T>
	void write(const T value) {
		T temp = value;
		buffer.resize(buffer.size() + sizeof(T));
		std::copy(reinterpret_cast<const char*>(&temp), reinterpret_cast<const char*>(&temp) + sizeof(T), buffer.end() - sizeof(T));
	}

	void write_string(const std::string& string) {
		buffer.resize(buffer.size() + string.size());
		std::copy(string.begin(), string.end(), buffer.end() - string.size());
	}

	template<typename T>
	void write_vector(const std::vector<T>& vector) {
		if constexpr (std::is_same_v<T, std::string>) {
			for (auto&& i : vector) {
				buffer.insert(buffer.end(), i.begin(), i.end());
			}
		} else {
			buffer.resize(buffer.size() + vector.size() * sizeof(T));
			std::copy(vector.begin(), vector.end(), buffer.end() - vector.size() * sizeof(T));
		}
	}
};
