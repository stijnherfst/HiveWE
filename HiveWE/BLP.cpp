#include "stdafx.h"

namespace blp {
	// Does not handle mipmaps, direct content, alpha bits or extra
	std::tuple<uint8_t*, uint32_t, uint32_t> BLP::load(const std::string& path) {
		BinaryReader reader(hierarchy.open_file(path));

		std::string magic_number = reader.readString(4);
		if (magic_number != "BLP1") {
			std::cout << "Could not load file as it is not a BLP1 file. Maybe it is BLP0 or BLP2." << std::endl;
			return { nullptr, 0, 0 };
		}

		uint32_t content = reader.read<uint32_t>();
		uint32_t alpha_bits = reader.read<uint32_t>();

		uint32_t width = reader.read<uint32_t>();
		uint32_t height = reader.read<uint32_t>();

		// Mipmaplocator
		uint32_t extra = reader.read<uint32_t>();
		bool has_mipmaps = reader.read<uint32_t>();
		std::vector<uint32_t> mipmap_offsets = reader.readVector<uint32_t>(16);
		std::vector<uint32_t> mipmap_sizes = reader.readVector<uint32_t>(16);

		uint32_t jpegHeaderSize = reader.read<uint32_t>();

		// Move header to be in front of the 0 level
		auto position = reader.buffer.begin() + reader.position;
		std::copy(position, position + jpegHeaderSize, reader.buffer.begin() + mipmap_offsets[0] - jpegHeaderSize);

		//std::vector<uint8_t> header = reader.readVector<uint8_t>(jpegHeaderSize);
		//reader.position = mipmap_offsets[0];
		//std::vector<uint8_t> mipmap = reader.readVector<uint8_t>(mipmap_sizes[0]);
		//header.insert(header.end(), mipmap.begin(), mipmap.end());

		unsigned char* buffer;
		if (content == CONTENT_JPEG) {
			// Decode JPEG content
			tjhandle handle = tjInitDecompress();
			buffer = new unsigned char[width * height * tjPixelSize[TJPF_CMYK]];
			int success = tjDecompress2(handle, reader.buffer.data() + mipmap_offsets[0] - jpegHeaderSize, jpegHeaderSize + mipmap_sizes[0], buffer, width, 0, height, TJPF_CMYK, 0); // Actually BGRA
			tjDestroy(handle);

			if (success == -1) {
				std::cout << "Error loading JPEG data from blp: " << tjGetErrorStr() << std::endl;
			}

			// BGRA to RGBA
 		//	for (size_t i = 0; i < width * height; i++) {
			//	unsigned char temp = buffer[i * 4];
			//	buffer[i * 4] = buffer[i * 4 + 2];
			//	buffer[i * 4 + 2] = temp;
			//}
		} else if (content == CONTENT_DIRECT) {
			std::cout << "Direct content for blp loading not yet implemented" << std::endl;
		}

		return { buffer, width, height };
	}
}

