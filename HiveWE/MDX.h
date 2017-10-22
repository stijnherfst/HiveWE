#pragma once

namespace mdx {
	class MDX {
	public:
		MDX(std::string path);
		void load(const std::string& path);
	};
}