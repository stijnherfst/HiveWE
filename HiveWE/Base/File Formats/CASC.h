#pragma once

// A thin wrapper around CascLib https://github.com/ladislav-zezula/CascLib
namespace casc {
	class File {
	public:
		HANDLE handle = nullptr;

		File() = default;
		~File();
		File(File&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
		}
		File(const File&) = default;
		File& operator=(const File&) = default;
		File& operator=(File&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
			return *this;
		}


		std::vector<uint8_t> read() const;
		size_t size() const noexcept;
		void close() const noexcept;
	};

	class CASC {
	public:
		HANDLE handle = nullptr;

		CASC() = default;

		explicit CASC(const fs::path& path);

		~CASC();
		CASC(CASC&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
		}
		CASC(const CASC&) = default;
		CASC& operator=(const CASC&) = delete;
		CASC& operator=(CASC&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
			return *this;
		}

		void open(const fs::path& path);
		void close();

		File file_open(const fs::path& path) const;
		bool file_exists(const fs::path& path) const;
	};
}