export module UnorderedMap;

import std;
import "ankerl/unordered_dense.h";

struct string_hash {
    using is_transparent = void; // enable heterogeneous lookup
    using is_avalanching = void; // mark class as high quality avalanching hash

    [[nodiscard]] auto operator()(const char* str) const noexcept -> uint64_t {
        return ankerl::unordered_dense::hash<std::string_view>{}(str);
    }

    [[nodiscard]] auto operator()(std::string_view str) const noexcept -> uint64_t {
        return ankerl::unordered_dense::hash<std::string_view>{}(str);
    }

    [[nodiscard]] auto operator()(std::string const& str) const noexcept -> uint64_t {
        return ankerl::unordered_dense::hash<std::string_view>{}(str);
    }
};


export namespace hive {
    template <typename T>
    concept StringLike = std::same_as<T, std::string>;

	/// Under the hood this is an ankerl::unordered_dense::map
	/// This template makes it pick the heterogeneous lookup version when the Key is std::string and otherwise uses the default
    template <typename Key, typename Value>
    using unordered_map = std::conditional_t<
        StringLike<Key>,
        ankerl::unordered_dense::map<Key, Value, string_hash, std::equal_to<>>,
        ankerl::unordered_dense::map<Key, Value>
    >;
}