#pragma once

namespace ini {
	class INI {
	public:
		// header to items to list of values to value
		std::map<std::string, std::map<std::string, std::vector<std::string>>> ini_data;

		INI() = default;
		explicit INI(const fs::path& path);

		void load(const fs::path& path);
		void substitute(const INI& ini, const std::string& section);

		std::map<std::string, std::vector<std::string>> section(const std::string& section) const;

		/// To access key data where the value of the key is comma seperated
		std::string data(const std::string& section, const std::string& key, const int argument = 0) const;

		/// Retrieves the list of key values
		std::vector<std::string> whole_data(const std::string& section, const std::string& key) const;

		bool key_exists(const std::string& section, const std::string& key);
	};
}