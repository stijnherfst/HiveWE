#pragma once

namespace blp {
	enum Content {
		jpeg,
		direct
	};

	class BLP {
	public:
		std::vector<std::tuple<int, int, std::vector<uint8_t>>> mipmaps;

		Content content;
		int alpha_bits;

		int width;
		int height;

		BLP(BinaryReader& reader);

		//static std::tuple<uint8_t*, uint32_t, uint32_t> load(const fs::path& path);
	};

}