export module BLP;

import std;
import types;
import BinaryReader;
import <turbojpeg.h>;

namespace blp {
	export struct Image {
		int width = 0;
		int height = 0;
		int channels = 4;
		std::vector<u8> data;
	};

	/// Decodes a BLP1 texture. Returns the decoded RGBA image
	export std::expected<Image, std::string> load(BinaryReader& reader) {
		try {
			const std::string magic_number = reader.read_string(4);
			if (magic_number != "BLP1") {
				return std::unexpected(std::format("Wrong magic number, expected BLP1 but got {}", magic_number));
			}

			const int content_type = reader.read<u32>();
			const int alpha_bits = reader.read<u32>();

			const u32 file_width = reader.read<u32>();
			const u32 file_height = reader.read<u32>();

			if (file_width == 0 || file_height == 0 || file_width > 32768 || file_height > 32768) {
				return std::unexpected(std::format("Invalid BLP dimensions {}x{}", file_width, file_height));
			}

			Image image;
			image.width = static_cast<int>(file_width);
			image.height = static_cast<int>(file_height);
			image.channels = 4;

			// extra and has_mipmaps
			reader.advance(8);

			const size_t pixel_count = static_cast<size_t>(image.width) * static_cast<size_t>(image.height);
			image.data.resize(pixel_count * image.channels);

			const auto mipmap_offsets = reader.read_vector<u32>(16);
			const auto mipmap_sizes = reader.read_vector<u32>(16);

			if (content_type == 0) { // jpeg
				tjhandle handle = tjInitDecompress();
				const u32 header_size = reader.read<u32>();

				// Validate offsets before any pointer arithmetic: the header must fit before the first
				// mipmap (so mipmap_offsets[0] - header_size can't underflow), and both the header and
				// the mipmap data must lie within the buffer.
				if (header_size > mipmap_offsets[0] || reader.position + header_size > reader.buffer.size()
					|| static_cast<size_t>(mipmap_offsets[0]) + mipmap_sizes[0] > reader.buffer.size()) {
					tjDestroy(handle);
					return std::unexpected<std::string>("Corrupt BLP JPEG offsets/sizes");
				}

				const auto header_position = reader.buffer.begin() + reader.position;

				// Move header in front of content
				std::copy(header_position, header_position + header_size, reader.buffer.begin() + mipmap_offsets[0] - header_size);

				const int success = tjDecompress2(handle, reader.buffer.data() + mipmap_offsets[0] - header_size, header_size + mipmap_sizes[0], image.data.data(), image.width, 0, image.height, TJPF_CMYK, 0); // Actually BGRA
				tjDestroy(handle);

				if (success == -1) {
					return std::unexpected(std::format("Error decoding BLP JPEG data: {}", tjGetErrorStr()));
				}
			} else if (content_type == 1) { // direct
				const auto header = reader.read_vector<u32>(256);

				// There might be fake mipmaps or the first mipmap could start within the 256 bytes of the colour header
				// Thus we cannot rely purely on advancing the position by mipmap sizes alone
				reader.position = mipmap_offsets[0];
				const auto rgb = reader.read_vector<u8>(pixel_count);

				if (alpha_bits == 0) {
					for (size_t j = 0; j < rgb.size(); j++) {
						// + (255 << 24) because the header alpha value is always 0 so we add 255
						reinterpret_cast<u32*>(image.data.data())[j] = header[rgb[j]] + (255 << 24);
					}
				} else {
					const auto alpha = reader.read_vector<u8>((pixel_count * alpha_bits + 7) / 8);

					for (size_t j = 0; j < rgb.size(); j++) {
						reinterpret_cast<u32*>(image.data.data())[j] = header[rgb[j]];
						switch (alpha_bits) {
							case 8:
								image.data[j * 4 + 3] = alpha[j];
								break;
							case 4: {
								const u8 byte = alpha[j / 2];
								image.data[j * 4 + 3] = (j % 2 ? byte >> 4 : byte & 0b00001111) * 17;
								break;
							}
							case 1:
								const bool bit = alpha[j / 8] & (1 << (j % 8));
								image.data[j * 4 + 3] = bit ? 255 : 0;
								break;
						}
					}
				}
			} else {
				return std::unexpected(std::format("Unsupported BLP content type {}", content_type));
			}

			// Data is BGR(A) instead of RGB(A). While GPUs can natively load BGRA some of the code would have to deal with both RGBA and BGRA which is a pita
			for (size_t i = 0; i < pixel_count; i++) {
				std::swap(image.data[i * image.channels], image.data[i * image.channels + 2]);
			}

			return image;
		} catch (const std::exception& e) {
			// BinaryReader throws std::out_of_range when the file is truncated; surface it as an error.
			return std::unexpected(std::format("Truncated or invalid BLP: {}", e.what()));
		}
	}
} // namespace blp
