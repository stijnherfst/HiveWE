#pragma once

#include "BinaryReader.h"

namespace blp {
	enum class Content {
		jpeg,
		direct
	};

	class BLP {
	public:
		Content content;
		int alpha_bits;

		int width;
		int height;
		std::vector<uint8_t> data;

		BLP(BinaryReader& reader);
	};

}