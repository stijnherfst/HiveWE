#pragma once

namespace ini {
	class INI {
	public:
		/// header to items to list of values to value
		std::map<std::string, std::map<std::string, std::vector<std::string>>> ini_data;

		INI() = default;
		explicit INI(const fs::path& path);

		void load(const fs::path& path);
		void substitute(const INI& ini, const std::string& section);

		std::map<std::string, std::vector<std::string>> section(const std::string& section) const;

		/// To access key data where the value of the key is comma seperated
		template<typename T = std::string>
		T data(const std::string & section, const std::string & key, const int argument = 0) const {
			if (ini_data.contains(section) && ini_data.at(section).contains(key) && argument < ini_data.at(section).at(key).size()) {
				if constexpr (std::is_same<T, std::string>()) {
					return ini_data.at(section).at(key)[argument];
				} else if constexpr (std::is_same<T, int>()) {
					return std::stoi(ini_data.at(section).at(key)[argument]);
				} else if constexpr (std::is_same<T, float>()) {
					return std::stof(ini_data.at(section).at(key)[argument]);
				} 
				static_assert("Type not supported. Convert yourself or add conversion here if it makes sense");
			} else {
				return T();
			}
		}

		/// Retrieves the list of key values
		std::vector<std::string> whole_data(const std::string& section, const std::string& key) const;

		/// Sets the data of a whole key
		void set_whole_data(const std::string& section, const std::string& key, const std::string& value);

		bool key_exists(const std::string& section, const std::string& key) const;
	};
}