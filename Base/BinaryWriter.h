#pragma once

class BinaryWriter {
public:
	std::vector<uint8_t> buffer;

	template<typename T>
	void write(const T value) {
		// These wouldn't make sense
		static_assert(std::is_same<T, std::string>() == false);
		static_assert(std::is_same<T, fs::path>() == false);

		T temp = static_cast<T>(value);
		buffer.resize(buffer.size() + sizeof(T));
		std::copy(reinterpret_cast<const char*>(&temp), reinterpret_cast<const char*>(&temp) + sizeof(T), buffer.end() - sizeof(T));
	}

	/// Writes the string to the buffer (null terminated if the input string is null terminated)
	void write_string(const std::string& string) {
		buffer.resize(buffer.size() + string.size());
		std::copy(string.begin(), string.end(), buffer.end() - string.size());
	}

	/// Writes a null terminated string to the buffer
	void write_c_string(const std::string& string) {
		if (!string.empty() && string.back() == '\0') {
			buffer.resize(buffer.size() + string.size());
			std::copy(string.begin(), string.end(), buffer.end() - string.size());
		} else {
			buffer.resize(buffer.size() + string.size() + 1);
			std::copy(string.begin(), string.end(), buffer.end() - string.size() - 1);
			buffer[buffer.size() - 1] = '\0';
		}
	}

	/// Copies the contents of the array to the buffer, has special code for std::string
	template<typename T>
	void write_vector(const std::vector<T>& vector) {
		if constexpr (std::is_same_v<T, std::string>) {
			for (const auto& i : vector) {
				buffer.insert(buffer.end(), i.begin(), i.end());
			}
		} else {
			buffer.resize(buffer.size() + vector.size() * sizeof(T));
			std::copy(vector.begin(), vector.end(), buffer.end() - vector.size() * sizeof(T));
		}
	}
};