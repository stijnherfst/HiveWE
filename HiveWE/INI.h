#pragma once

namespace ini {
	class INI {
	
		// header to items to content
		std::map<std::string, std::map<std::string, std::string>> ini_data;

	public:
		INI() = default;
		INI(const fs::path& path);

		void load(fs::path path);

		std::map<std::string, std::string> section(std::string section);
		std::string data(std::string section, std::string key);
	};
}