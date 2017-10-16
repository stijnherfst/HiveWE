#pragma once

namespace blp {
	enum Content {
		CONTENT_JPEG,
		CONTENT_DIRECT
	};

	class BLP {
	public:
		BLP(const std::string& path) {

		}

		static uint8_t* load(const std::string& path);
	};

}