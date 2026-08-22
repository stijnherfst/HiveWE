module;

#include "ankerl/unordered_dense.h"

#ifdef __linux__
#include "std_compat.h"
#endif

export module StringHash;

#ifndef __linux__
import std;
#endif

export struct string_hash {
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
}
