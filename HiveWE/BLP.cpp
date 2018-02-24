#include "stdafx.h"

namespace blp {
	// Does not handle mipmaps, or extra
	std::tuple<uint8_t*, uint32_t, uint32_t> BLP::load(const fs::path& path) {
		BinaryReader reader = hierarchy.open_file(path);

		const std::string magic_number = reader.read_string(4);
		if (magic_number != "BLP1") {
			std::cout << "Could not load file as it is not a BLP1 file. Maybe it is BLP0 or BLP2." << std::endl;
			return { nullptr, 0, 0 };
		}

		const uint32_t content = reader.read<uint32_t>();
		uint32_t alpha_bits = reader.read<uint32_t>();

		uint32_t width = reader.read<uint32_t>();
		uint32_t height = reader.read<uint32_t>();

		// Mipmaplocator
		uint32_t extra = reader.read<uint32_t>();
		bool has_mipmaps = reader.read<uint32_t>();
		std::vector<uint32_t> mipmap_offsets = reader.read_vector<uint32_t>(16);
		std::vector<uint32_t> mipmap_sizes = reader.read_vector<uint32_t>(16);

		uint8_t* buffer;
		if (content == CONTENT_JPEG) {
			const uint32_t header_size = reader.read<uint32_t>();
			// Move header to be in front of the 0 level
			const auto position = reader.buffer.begin() + reader.position;
			std::copy(position, position + header_size, reader.buffer.begin() + mipmap_offsets[0] - header_size);

			// Decode JPEG content
			tjhandle handle = tjInitDecompress();
			buffer = new uint8_t[width * height * tjPixelSize[TJPF_CMYK]];
			const int success = tjDecompress2(handle, reader.buffer.data() + mipmap_offsets[0] - header_size, header_size + mipmap_sizes[0], buffer, width, 0, height, TJPF_CMYK, 0); // Actually BGRA
			tjDestroy(handle);

			if (success == -1) {
				std::cout << "Error loading JPEG data from blp: " << tjGetErrorStr() << std::endl;
			}
		} else if (content == CONTENT_DIRECT) {
			std::vector<uint32_t> header = reader.read_vector<uint32_t>(256);
			std::vector<uint8_t> data = reader.read_vector<uint8_t>(width * height);
			
			//if (alpha_bits > 0) {
			//	std::vector<uint8_t> alpha_data = reader.read_vector<uint8_t>((width * height * alpha_bits + 7) / 8);
			//}

			buffer = new uint8_t[width * height * 4];
			// ToDo handle alpha content
			for (size_t i = 0; i < data.size(); i++) {
				reinterpret_cast<uint32_t*>(buffer)[i] = header[data[i]];
				buffer[i * 4 + 3] = 255;
			}
		}

		

		return { buffer, width, height };
	}
}

