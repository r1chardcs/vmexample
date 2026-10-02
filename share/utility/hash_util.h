//
// Created by dlllibstdntc on 01.10.2026.
//

#ifndef MYVM_HASH_UTIL_H
#define MYVM_HASH_UTIL_H

#include <string>
#include <share/utility/base_types.h>

namespace share {
    u8 computeHash(const std::wstring &str);
    u8 computeHash(const std::string &str);

    inline u8 computeHash(const std::wstring &str) {
        u8 h = 1469598103934665603ull;
        for (const auto c : str) {
            h ^= c;
            h *= 1099511628211ull;
        }
        return h;
    }

    inline u8 computeHash(const std::string &str) {
        u8 h = 1469598103934665603ull;
        for (const auto c : str) {
            h ^= c;
            h *= 1099511628211ull;
        }
        return h;
    }
}

#endif //MYVM_HASH_UTIL_H
