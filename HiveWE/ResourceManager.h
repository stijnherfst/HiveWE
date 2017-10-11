#pragma once

class Resource {
public:
	Resource(const std::string&) {};
	virtual ~Resource() {};
};

class Texture : public Resource {
public:
	int width;
	int height;
	int channels;
	uint8_t* data;

	Texture(const std::string& path) : Resource(path) {
		load(path);
	}

	virtual ~Texture() {
		unload();
	}
protected:
	void load(const std::string& path);

	void unload();
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
private:
	std::unordered_map<std::string, std::weak_ptr<Resource>> resources;
};

extern ResourceManager resource_manager;