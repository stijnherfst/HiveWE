
module;

#include <QObject>

module Map;

import std;
import MDX;

namespace fs = std::filesystem;

std::vector<FileUsage> Map::get_file_usage() const {
	// First, get all .MDX files referenced by the map
	// Then load each of them and save the resources references by them
	// Deal with game overrides somehow

	const auto total_start = std::chrono::steady_clock::now();
	auto start = std::chrono::steady_clock::now();

	hive::unordered_map<std::string, std::unordered_set<std::string>> resources;

	const auto normalize_path = [&](const std::string& path) {
		auto path_copy = path;
		if (path_copy.ends_with(".mdl")) {
			path_copy = path.substr(0, path.size() - 4) + ".mdx";
		}
		normalize_path_to_forward_slash(path_copy);
		return path_copy;
	};

	const auto find_references = [&](const slk::SLK& slk, const std::vector<std::string>& keys) {
		for (const auto& [id, values] : slk.shadow_data) {
			for (const auto& key : keys) {
				if (auto found = values.find(key); found != values.end()) {
					resources[normalize_path(found->second)].emplace(id);
				}
			}
		}
	};

	find_references(units_slk, {"file", "portrait", "specialart", "missileart1", "missileart2", "art", "pathtex"});
	find_references(items_slk, {"file", "art"});
	find_references(destructibles_slk, {"file", "pathtex", "pathtexdeath"});
	find_references(doodads_slk, {"file", "pathtex"});
	find_references(abilities_slk, {"targetart", "effectart", "specialart", "art", "researchart"});
	find_references(buff_slk, {"targetart", "missileart", "specialart", "buffart"});
	// Todo, all icon levels
	// find_references(upgrade_slk, {"file"});

	if (!info.loading_screen_model.empty() && info.loading_screen_number == -1) {
		resources[normalize_path(info.loading_screen_model)].emplace("loadingscreen");
	}

	for (const auto& i : sounds.sounds) {
		resources[normalize_path(i.file)].emplace(i.name);
	}

	std::println(
		"Find references: {:>5}ms",
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()
	);
	start = std::chrono::steady_clock::now();

	hive::unordered_map<std::string, std::unordered_set<std::string>> referenced_resources;
	std::mutex mutex;

	std::for_each(std::execution::par_unseq, resources.begin(), resources.end(), [&](const auto& resource) {
		const auto& [path, ids] = resource; // MSVC does not support structured bindings in lambdas atm.

		if (!path.ends_with(".mdx")) {
			return;
		}

		mdx::MDX mdx;
		try {
			auto mdx_content = hierarchy.open_file(path);
			if (!mdx_content) {
				std::println("Error loading mdx: {} with error: {}", path, mdx_content.error());
				return;
			}
			mdx = mdx::MDX(mdx_content.value());
		} catch (const std::exception& e) {
			std::println("Exception loading mdx: {} with error: {}", path, e.what());
			return;
		}

		const auto guard = std::scoped_lock(mutex);
		for (const auto& texture : mdx.textures) {
			referenced_resources[normalize_path(texture.file_name.string())].emplace(path);
		}

		for (const auto& attachment : mdx.attachments) {
			referenced_resources[normalize_path(attachment.path)].emplace(path);
		}

		for (const auto& emitter : mdx.emitters1) {
			referenced_resources[normalize_path(emitter.path)].emplace(path);
		}
	});

	std::println(
		"Find MDX references: {:>5}ms",
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()
	);
	start = std::chrono::steady_clock::now();

	for (const auto& [path, values] : referenced_resources) {
		resources[path].insert(values.begin(), values.end());
	}

	std::unordered_set<std::string> files;

	for (const auto& i : fs::recursive_directory_iterator(filesystem_path)) {
		if (i.is_regular_file()) {
			const auto new_path = i.path();
			std::string path = new_path.lexically_relative(filesystem_path).string();
			std::string file_name = i.path().filename().string();
			if (imports.blacklist.contains(file_name)) {
				continue;
			}
			files.emplace(normalize_path(path));
		}
	}

	std::string script_file_name;
	if (info.lua) {
		script_file_name = "war3map.lua";
	} else {
		script_file_name = "war3map.j";
	}

	const auto binary = read_file(filesystem_path / script_file_name);
	std::string map_script;
	if (!binary) {
		std::println("Error reading map script. Save your map first.");
	} else {
		const auto a = binary.value();
		map_script = std::string(a.buffer.begin(), a.buffer.end());
	}

	std::vector<FileUsage> result;
	result.reserve(files.size());
	for (const auto& file : files) {
		FileUsage usage;
		usage.path = file;
		if (resources.contains(file)) {
			usage.used_by = resources.at(file);
		} else if (!map_script.empty() && map_script.contains(file)) {
			usage.used_by.emplace("map script");
		}
		result.push_back(std::move(usage));
	}

	std::println(
		"Find unused files: {:>5}ms",
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()
	);
	std::println(
		"Total: {:>5}ms",
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - total_start).count()
	);

	return result;
}
