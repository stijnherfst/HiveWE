#pragma once

class Resource {
public:
	virtual ~Resource() {};
};

class ResourceManager {
public:
	template<typename T>
	std::shared_ptr<T> load(const std::string path, bool local = false) {
		std::string resource = path + " "s + T::name;

		static_assert(std::is_base_of<Resource, T>::value, "T must inherit from Resource");

		auto res = resources[resource].lock();
		if (!res) {
			resources[resource] = res = std::make_shared<T>(path);
		}

		return std::dynamic_pointer_cast<T>(res);
	}

	template<typename T>
	std::shared_ptr<T> load(const std::initializer_list<std::string> paths, bool local = false) {
		std::string resource;
		for (auto&& path : paths) {
			resource += path;
		}
		resource += " "s + T::name;

		static_assert(std::is_base_of<Resource, T>::value, "T must inherit from Resource");

		auto res = resources[resource].lock();
		if (!res) {
			resources[resource] = res = std::make_shared<T>(paths);
		}

		return return_value = std::dynamic_pointer_cast<T>(res);
	}
private:
	std::unordered_map<std::string, std::weak_ptr<Resource>> resources;
};

extern ResourceManager resource_manager;