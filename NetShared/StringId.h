#pragma once
#include <string>
#include <cstdint>

using StringId = uint32_t;

constexpr inline StringId CompileTimeStringId(const char* str, uint32_t hash = 2166136261u) {
    return *str ? CompileTimeStringId(str + 1, (hash ^ *str) * 16777619u) : hash;
}

constexpr inline StringId operator"" _sid(const char* str, size_t) {
    return CompileTimeStringId(str);
}

inline StringId RuntimeStringId(const std::string& str) {
    return CompileTimeStringId(str.c_str());
}