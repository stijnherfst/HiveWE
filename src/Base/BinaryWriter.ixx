module;

#include <cstring>
#include <string>
#include <vector>

export module BinaryWriter;

export class BinaryWriter {
  public:
	std::vector<uint8_t> buffer;

	template <typename T = void, typename U>
	void write(U value) {
		static_assert(std::is_standard_layout<U>::value, "U must be of standard layout.");

		if constexpr (not std::is_void_v<T>) {
			static_assert(std::is_standard_layout<T>::value, "T must be of standard layout.");

			T temp = static_cast<T>(value);
			buffer.resize(buffer.size() + sizeof(T));
			std::memcpy(buffer.data() + buffer.size() - sizeof(T), &temp, sizeof(T));
		} else {
			U temp = value;
			buffer.resize(buffer.size() + sizeof(U));
			std::memcpy(buffer.data() + buffer.size() - sizeof(U), &temp, sizeof(U));
		}
	}

	/// Writes the string to the buffer (null terminated if the input string is null terminated)
	/// ToDo string_view?
	void write_string(const std::string& string) {
		buffer.resize(buffer.size() + string.size());
		std::copy(string.begin(), string.end(), buffer.end() - string.size());
	}

	/// Writes a null terminated string to the buffer
	/// ToDo string_view?
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

	/// Writes a null terminated string to the buffer with padding
	/// ToDo string_view?
	void write_c_string_padded(const std::string& string, int final_size) {
		buffer.resize(buffer.size() + final_size);
		std::copy(string.begin(), string.end(), buffer.end() - final_size);

		// std::vector::resize will memset to 0 so adding a \0 terminator is not required
	}

	/// Copies the contents of the array to the buffer, has special code for std::string
	template <typename T>
	void write_vector(const std::vector<T>& vector) {
		if constexpr (std::is_same_v<T, std::string>) {
			for (const auto& i : vector) {
				buffer.insert(buffer.end(), i.begin(), i.end());
			}
		} else {
			static_assert(std::is_standard_layout<T>::value, "T must be of standard layout or std::string.");
			buffer.resize(buffer.size() + vector.size() * sizeof(T));
			// std::copy(vector.begin(), vector.end(), buffer.end() - vector.size() * sizeof(T));
			std::memcpy(buffer.data() + buffer.size() - vector.size() * sizeof(T), vector.data(), vector.size() * sizeof(T));
		}
	}
};