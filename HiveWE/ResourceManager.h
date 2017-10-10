#pragma once

class Resource {
public:
	Resource() {};
	virtual ~Resource() {};


	virtual void load(const std::string& path) = 0;
	virtual void unload() = 0;

protected:
	unsigned mResourceId;
	std::string mResourcePath;

};

struct resource_free {
	void operator()(Resource* resource) {
		resource->unload();
		delete resource;
	}
};

class Texture : public Resource {
public:
	int width;
	int height;
	int channels;
	unsigned char* data;

	void load(const std::string& path);
	void unload();
};

class ResourceManager {
public:
	template <typename T>
	T& load(const std::string& name) {
		static_assert(std::is_base_of<Resource, T>::value, "T must inherit from Resource");
		
		auto found = resources.find(name);
		if (found == resources.end()) {
			T* resource_instance = new T;
			Resource* resource_base = dynamic_cast<Resource*>(resource_instance);
			resource_base->load(name);
			resources.emplace(name, resource_base);

			return *resource_instance;
		} else {
			return *dynamic_cast<T*>(found->second.get());
		}
	}

	void unload(const std::string& path) {
		resources.erase(path);
	}
private:
	std::unordered_map<std::string, std::unique_ptr<Resource, resource_free>> resources;
};

extern ResourceManager resource_manager;