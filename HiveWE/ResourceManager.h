#pragma once

class Resource {
public:
	virtual ~Resource() {};
};

class ResourceManager {
public:
	template<typename T>
	std::shared_ptr<T> load(const std::string& path) {
		// Clean path? C:/ vs C:\\

		static_assert(std::is_base_of<Resource, T>::value, "T must inherit from Resource");

		auto res = resources[path].lock();
		if (!res) {
			resources[path] = res = std::make_shared<T>(path);
		}

		auto return_value = std::dynamic_pointer_cast<T>(res);
		if (!return_value) {
			throw std::runtime_error("Resource "s + path + " is already loaded as another type"s);
		}
		return return_value;
	}

	template<typename T>
	std::shared_ptr<T> load(const std::initializer_list<std::string> paths) {
		// Clean path? C:/ vs C:\\

		std::string resource;
		for (auto&& path : paths) {
			resource += path;
		}

		static_assert(std::is_base_of<Resource, T>::value, "T must inherit from Resource");

		auto res = resources[resource].lock();
		if (!res) {
			resources[resource] = res = std::make_shared<T>(paths);
		}

		auto return_value = std::dynamic_pointer_cast<T>(res);
		if (!return_value) {
			throw std::runtime_error("Resource "s + resource + " is already loaded as another type"s);
		}
		return return_value;
	}
private:
	std::unordered_map<std::string, std::weak_ptr<Resource>> resources;
};

extern ResourceManager resource_manager;