#pragma once

namespace ini {
	class INI {
		// header to items to content
		std::map<std::string, std::map<std::string, std::string>> ini_data;

	public:
		INI() = default;
		INI(const fs::path& path);

		void load(const fs::path& path);
		void substitute(const INI& ini, const std::string& section);

		std::map<std::string, std::string> section(const std::string& section) const;
		std::string data(const std::string& section, const std::string& key) const;
	};
}