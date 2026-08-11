export module UnorderedSet;

import std;
import "ankerl/unordered_dense.h";
import StringHash;

export namespace hive {
	/// Under the hood this is an ankerl::unordered_dense::set
	/// This template makes it pick the heterogeneous lookup version when the Key is std::string and otherwise uses the default
    template <typename Key>
    using unordered_set = std::conditional_t<
        StringLike<Key>,
        ankerl::unordered_dense::set<Key, string_hash, std::equal_to<>>,
        ankerl::unordered_dense::set<Key>
    >;
}
