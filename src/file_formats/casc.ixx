export module CASC;

import std;
import types;
import no_init_allocator;
import BinaryReader;
import <whiteout/storages/casc/storage.h>;

namespace fs = std::filesystem;

// A thin wrapper around WhiteoutLib's CASC reader https://github.com/FernandoS27/WhiteoutLib
namespace casc {
	export class CASC {
		std::optional<whiteout::storages::casc::Storage> storage;

	  public:
		/// Opens the storage of the Warcraft III installation in `directory`.
		/// `product` picks the build: "w3" for retail and "w3t" for the PTR
		bool open(const fs::path& directory, const std::string& product) {
			std::string error;
			storage = whiteout::storages::casc::Storage::open({
				.path = directory.string(),
				.product = product,
				.errorOut = &error,
			});
			if (!storage) {
				std::print("Error opening {} ({}): {}\n", directory.string(), product, error);
			}
			return storage.has_value();
		}

		void close() {
			storage.reset();
		}

		/// The paths matching `mask`, in which `*` matches any run of characters (path separators included) and `?` one character
		std::vector<std::string> find_files(const std::string& mask) const {
			std::vector<std::string> files;
			storage->enumerate(mask, [&](const whiteout::storages::casc::EnumerateEntry& entry) {
				files.emplace_back(entry.path);
				return true;
			});
			return files;
		}

		[[nodiscard]] std::expected<BinaryReader, std::string> open_file(const std::string& path) const {
			const auto data = storage->readFile(path);
			if (!data) {
				return std::unexpected(std::format("Error opening {} with error: {}\n", path, whiteout::storages::casc::Storage::lastError()));
			}
			std::vector<u8, default_init_allocator<u8>> buffer(data->size());
			std::memcpy(buffer.data(), data->data(), data->size());
			return BinaryReader(std::move(buffer));
		}

		/// Whether the file is listed in the storage, which lists all locales regardless of what is installed
		bool file_exists(const std::string& path) const {
			return storage->fileExists(path);
		}

		/// Whether the file's data is actually downloaded/available locally, as opposed to merely
		/// being present in the storage index (which lists all locales regardless of what is installed).
		/// Only meant for small files, as it reads the file
		bool file_exists_locally(const std::string& path) const {
			return storage->readFile(path).has_value();
		}
	};
} // namespace casc
