#pragma once
//Only currently used for filealiases.json
namespace json {
	class JSON {
	public:
		std::map<std::string, std::string> json_data;


		JSON() = default;
		explicit JSON(const fs::path& path);

		void load(const fs::path& path);

		bool exists(const std::string& file) const;

		std::string alias(const std::string& file) const;
	};
}