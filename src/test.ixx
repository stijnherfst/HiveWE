module;

#include <string>
#include <vector>
#include <fstream>
#include <execution>
#include <filesystem>
#include <print>

#include <glm/glm.hpp>

export module test;

namespace fs = std::filesystem;

import BinaryReader;
import MDX;
import no_init_allocator;

void parse_all_mdx() {
	std::vector<fs::path> paths;

	for (const auto i : fs::recursive_directory_iterator("D:/Warcraft/WC3/Assets")) {
		if (i.is_regular_file() && (i.path().extension() == ".mdx" || i.path().extension() == ".MDX")) {
			paths.push_back(i.path());
		}
	}

	auto min = glm::vec3(99999.f, 99999.f, 99999.f);
	auto max = glm::vec3(-99999.f, -99999.f, -99999.f);

	std::for_each(std::execution::seq, paths.begin(), paths.end(), [&](const fs::path& path) {
		std::ifstream stream(path, std::ios::binary);
		auto buffer = std::vector<uint8_t, default_init_allocator<uint8_t>>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

		BinaryReader reader(buffer);
		auto mdx = mdx::MDX(reader);
		
		//min = glm::min(min, mdx.extent.minimum);
		//max = glm::max(min, mdx.extent.maximum);

		//std::println("x: {} y: {} z: {}", (max - min).x, (max - min).y, (max - min).z);

		//min = mdx.extent.minimum / 128.f;
		//max = mdx.extent.maximum / 128.f;

		//std::println("x: {} y: {} z: {} path: {}", (max - min).x, (max - min).y, (max - min).z, path.string());
	});
}

export void execute_tests() {
	std::print("[INFO] Parsing all MDX files\n");
	auto begin = std::chrono::steady_clock::now();
	parse_all_mdx();
	auto delta = (std::chrono::steady_clock::now() - begin).count() / 1'000'000.f;
	std::print("[INFO] Done parsing in {}ms\n", delta);
}
