#pragma once

#include "BinaryReader.h"

namespace blp {
	uint8_t* load(BinaryReader& reader, int& width, int& height, int& channels);
}