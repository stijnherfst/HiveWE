module;

#include <cmath>
#include <print>

#include <turbojpeg.h>

export module BLP;

import BinaryReader;

namespace blp {
	export uint8_t* load(BinaryReader& reader, int& width, int& height, int& channels) {
		const std::string magic_number = reader.read_string(4);
		if (magic_number != "BLP1") {
			std::print("Wrong magic number, should be BLP1, is {}\n", magic_number);
			return nullptr;
		}

		int content_type = reader.read<uint32_t>();
		int alpha_bits = reader.read<uint32_t>();

		width = reader.read<uint32_t>();
		height = reader.read<uint32_t>();
		channels = 4;

		// extra and has_mipmaps
		reader.advance(8);

		uint8_t* data = new uint8_t[width * height * 4];

		auto mipmap_offsets = reader.read_vector<uint32_t>(16);
		auto mipmap_sizes = reader.read_vector<uint32_t>(16);

		if (content_type == 0) { // jpeg
			tjhandle handle = tjInitDecompress();
			const uint32_t header_size = reader.read<uint32_t>();
			auto header_position = reader.buffer.begin() + reader.position;

			// Move header in front of content
			std::copy(header_position, header_position + header_size, reader.buffer.begin() + mipmap_offsets[0] - header_size);
			header_position = reader.buffer.begin() + mipmap_offsets[0] - header_size;

			const int success = tjDecompress2(handle, reader.buffer.data() + mipmap_offsets[0] - header_size, header_size + mipmap_sizes[0], data, width, 0, height, TJPF_CMYK, 0); // Actually BGRA

			if (success == -1) {
				std::print("Error loading JPEG data from BLP {}\n", tjGetErrorStr());
			}
			tjDestroy(handle);
		} else if (content_type == 1) { // direct
			auto header = reader.read_vector<uint32_t>(256);

			// There might be fake mipmaps or the first mipmap could start within the 256 bytes of the colour header
			// Thus we cannot rely purely on advancing the position by mipmap sizes alone
			reader.position = mipmap_offsets[0];
			auto rgb = reader.read_vector<uint8_t>(width * height);

			if (alpha_bits == 0) {
				for (size_t j = 0; j < rgb.size(); j++) {
					// + (255 << 24) because the header alpha value is always 0 so we add 255
					reinterpret_cast<uint32_t*>(data)[j] = header[rgb[j]] + (255 << 24);
				}
			} else {
				auto alpha = reader.read_vector<uint8_t>((width * height * alpha_bits + 7) / 8);

				for (size_t j = 0; j < rgb.size(); j++) {
					reinterpret_cast<uint32_t*>(data)[j] = header[rgb[j]];
					switch (alpha_bits) {
						case 8:
							data[j * 4 + 3] = alpha[j];
							break;
						case 4: {
							uint8_t byte = alpha[j / 2];
							data[j * 4 + 3] = j % 2 ? byte >> 4 : byte & 0b00001111;
							break;
						}
						case 1:
							data[j * 4 + 3] = alpha[j / 8] & (1 << (j % 8));
							break;
					}
				}
			}
		}

		// Data is BGR(A) instead of RGB(A). While GPUs can natively load BGRA some of the code would have to deal with both RGBA and BGRA which is a pita
		for (int i = 0; i < width * height; i++) {
			std::swap(data[i * channels], data[i * channels + 2]);
		}

		return data;
	}
} // namespace blp