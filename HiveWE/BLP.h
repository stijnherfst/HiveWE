#pragma once

namespace blp {
	enum Content {
		CONTENT_JPEG,
		CONTENT_DIRECT
	};

	class BLP {
	public:
		//BLP(const std::string& path) {
		//}

		static std::tuple<uint8_t*, uint32_t, uint32_t> load(const fs::path& path);
	};

}