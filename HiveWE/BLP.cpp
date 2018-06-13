#include "stdafx.h"

namespace blp {
	BLP::BLP(BinaryReader& reader) {
		const std::string magic_number = reader.read_string(4);
		if (magic_number != "BLP1") {
			std::cout << "Could not load file as it is not a BLP1 file. Maybe it is BLP0 or BLP2.\n";
			return;
		}

		content = static_cast<Content>(reader.read<uint32_t>());
		alpha_bits = reader.read<uint32_t>();

		width = reader.read<uint32_t>();
		height = reader.read<uint32_t>();

		// extra and has_mipmaps
		reader.advance(8);

		// Mipmaplocator
		auto mipmap_offsets = reader.read_vector<uint32_t>(16);
		auto mipmap_sizes = reader.read_vector<uint32_t>(16);

		if (content == jpeg) {
			tjhandle handle = tjInitDecompress();
			const uint32_t header_size = reader.read<uint32_t>();
			auto header_position = reader.buffer.begin() + reader.position;

			for (int i = 0; i < mipmap_sizes.size(); i++) {
				if (mipmap_sizes[i] == 0) {
					goto exitloop;
				}
				int mipmap_width = std::max(1.0, width / std::pow(2, i));
				int mipmap_height = std::max(1.0, height / std::pow(2, i));
				mipmaps.emplace_back(mipmap_width, mipmap_height, std::vector<uint8_t>(mipmap_width * mipmap_height * 4));

				// Move header in front of content
				std::copy(header_position, header_position + header_size, reader.buffer.begin() + mipmap_offsets[i] - header_size);
				header_position = reader.buffer.begin() + mipmap_offsets[i] - header_size;

				const int success = tjDecompress2(handle, reader.buffer.data() + mipmap_offsets[i] - header_size, header_size + mipmap_sizes[i], std::get<2>(mipmaps.back()).data(), mipmap_width, 0, mipmap_height, TJPF_CMYK, 0); // Actually BGRA

				if (success == -1) {
					std::cout << "Error loading JPEG data from blp: " << tjGetErrorStr() << std::endl;
				}
			}
			exitloop:
			tjDestroy(handle);
		} else if (content == direct) {
			auto header = reader.read_vector<uint32_t>(256);

			for (int i = 0; i < mipmap_sizes.size(); i++) {
				if (mipmap_sizes[i] == 0) {
					goto exitloop2;
				}

				int mipmap_width = std::max(1.0, width / std::pow(2, i));
				int mipmap_height = std::max(1.0, height / std::pow(2, i));

				auto rgb = reader.read_vector<uint8_t>(mipmap_width * mipmap_height);

				mipmaps.emplace_back(mipmap_width, mipmap_height, std::vector<uint8_t>(mipmap_width * mipmap_height * 4, 255));
				if (alpha_bits == 0) {
					for (size_t j = 0; j < rgb.size(); j++) {
						reinterpret_cast<uint32_t*>(std::get<2>(mipmaps[i]).data())[j] = header[rgb[j]];
					}
				} else {
					auto alpha = reader.read_vector<uint8_t>((mipmap_width * mipmap_height * alpha_bits + 7) / 8);
					
					for (size_t j = 0; j < rgb.size(); j++) {
						reinterpret_cast<uint32_t*>(std::get<2>(mipmaps[i]).data())[j] = header[rgb[j]];
						switch (alpha_bits) {
							case 8:
								std::get<2>(mipmaps[i])[j * 4 + 3] = alpha[j];
								break;
							case 4: {
								uint8_t byte = alpha[j / 2];
								std::get<2>(mipmaps[i])[j * 4 + 3] = j % 2 ? byte >> 4 : byte & 0b00001111;
								break;
							}
							case 1:
								std::get<2>(mipmaps[i])[j * 4 + 3] = alpha[j / 8] & (1 << (j % 8));
								break;
						}
					}
				}
			}
			exitloop2:;
		}
	}
}