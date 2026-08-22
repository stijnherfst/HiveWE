module;

#include "ankerl/unordered_dense.h"

#ifdef __linux__
#include "std_compat.h"
#endif

export module UnorderedMap;

#ifndef __linux__
import std;
#endif
import StringHash;

export namespace hive {
	/// Under the hood this is an ankerl::unordered_dense::map
	/// This template makes it pick the heterogeneous lookup version when the Key is std::string and otherwise uses the default
    template <typename Key, typename Value>
    using unordered_map = std::conditional_t<
        StringLike<Key>,
        ankerl::unordered_dense::map<Key, Value, string_hash, std::equal_to<>>,
        ankerl::unordered_dense::map<Key, Value>
    >;
}