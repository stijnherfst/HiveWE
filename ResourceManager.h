#pragma once

class Resource {
public:
	virtual ~Resource() = default;
};

class ResourceManager {
public:
	template<typename T>
	std::shared_ptr<T> load(const fs::path& path) {
		static_assert(std::is_base_of<Resource, T>::value, "T must inherit from Resource");

		const std::string resource = path.string() + T::name;

		auto res = resources[resource].lock();
		if (!res) {
			resources[resource] = res = std::make_shared<T>(path);
		}

		return std::dynamic_pointer_cast<T>(res);
	}

	template<typename T>
	std::shared_ptr<T> load(const std::initializer_list<fs::path> paths) {
		static_assert(std::is_base_of<Resource, T>::value, "T must inherit from Resource");
		
		std::string resource;
		for (const auto& path : paths) {
			resource += path.string();
		}
		resource += T::name;

		auto res = resources[resource].lock();
		if (!res) {
			resources[resource] = res = std::make_shared<T>(paths);
		}

		return std::dynamic_pointer_cast<T>(res);
	}

private:
	std::unordered_map<std::string, std::weak_ptr<Resource>> resources;
};

extern ResourceManager resource_manager;