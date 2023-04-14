module;

#include <unordered_map>
#include <string>
#include <memory>
#include <filesystem>

export module ResourceManager;

namespace fs = std::filesystem;

export class Resource {
  public:
	virtual ~Resource() = default;
};

export class ResourceManager {
  public:
	/// Loads and caches a resource in memory until no longer referenced.
	/// Whether two load paths lead to different cached instances is determined by the path, T::name and custom_identifier
	/// Any additional arguments are passed to your type its constructor
	template <typename T, typename... Args>
	std::shared_ptr<T> load(const fs::path& path, const std::string& custom_identifier = "", Args... args) {
		static_assert(std::is_base_of<Resource, T>::value, "T must inherit from Resource");
		const std::string resource = path.string() + T::name + custom_identifier;

		auto res = resources[resource].lock();
		if (!res) {
			resources[resource] = res = std::make_shared<T>(path, args...);
		}

		return std::dynamic_pointer_cast<T>(res);
	}

	template <typename T>
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

export inline ResourceManager resource_manager;