#pragma once

//class Resource {
//public:
//	Resource() {};
//	virtual ~Resource() {};
//
//
//	virtual void load(const std::string& path) = 0;
//	virtual void unload() = 0;
//
//protected:
//	unsigned mResourceId;
//	std::string mResourcePath;
//
//};

//struct ResourceFree {
//	void operator()(Resource* resource) {
//		resource->unload();
//		delete resource;
//	}
//};

//class Texture : public Resource {
//public:
//	int width;
//	int height;
//	int channels;
//	unsigned char* data;
//
//	void load(const std::string& path);
//	void unload();
//};

//class ResourceManager {
//	public:
//		template <typename T>
//		T& load(const std::string& name) {
//			static_assert(std::is_base_of<Resource, T>::value, "T must inherit from Resource");
//		
//			auto found = resources.find(name);
//			if (found == resources.end()) {
//				T* resource_instance = new T;
//				Resource* resource_base = dynamic_cast<Resource*>(resource_instance);
//				resource_base->load(name);
//				resources.emplace(name, resource_base);
//
//				return *resource_instance;
//			} else {
//				return *dynamic_cast<T*>(found->second.get());
//			}
//		}
//
//		void unload(const std::string& path) {
//			resources.erase(path);
//		}
//	private:
//		std::unordered_map<std::string, std::unique_ptr<Resource, ResourceFree>> resources;
//	};


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

//class Resource {
//public:
//	Resource(const std::string&) {}
//	virtual ~Resource() {}
//};
//
//class Texture : public Resource {
//public:
//	int width;
//	int height;
//	int channels;
//	uint8_t* data;
//
//	Texture(const std::string& path) : Resource(path) {
//		load(path);
//	}
//
//	virtual ~Texture() {
//		// if needed for whatever reason
//		unload();
//	}
//protected:
//	void load(const std::string& path) {
//		// loading logic in here
//	}
//
//	void unload() {
//		// unloading logic in here
//	}
//};
//
//class ResourceManager {
//public:
//	template<typename T>
//	std::shared_ptr<T> load(const std::string& path) {
//		static_assert(std::is_base_of<Resource, T>::value, "T must inherit from Resource");
//
//		auto res = resources[path].lock();
//		if (!res) {
//			// assuming constructor loads resource
//			resources[path] = res = std::make_shared<T>(path);
//		}
//
//		auto return_value = std::dynamic_pointer_cast<T>(res);
//		if (!return_value) {
//			throw std::runtime_error(std::string("Resource '") + path + "' is already loaded as another type");
//		}
//		return return_value;
//	}
//private:
//	std::unordered_map<std::string, std::weak_ptr<Resource>> resources;
//};
//
//
