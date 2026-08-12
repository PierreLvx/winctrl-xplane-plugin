#ifndef PATH_HPP
#define PATH_HPP

#include <filesystem>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

// X-Plane hands us UTF-8 paths; a narrow std::filesystem::path decodes with the
// ANSI code page on Windows, so non-ASCII install paths need UTF-16.
inline std::filesystem::path utf8ToPath(const std::string &utf8) {
#if defined(_WIN32) || defined(_WIN64)
    if (utf8.empty()) {
        return {};
    }

    int length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (length <= 0) {
        return std::filesystem::path(utf8);
    }

    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), length);
    return std::filesystem::path(wide);
#else
    return std::filesystem::path(utf8);
#endif
}

// Inverse of utf8ToPath. Never throws, unlike path::string() on Windows.
inline std::string pathToUtf8(const std::filesystem::path &path) {
#if defined(_WIN32) || defined(_WIN64)
    const std::wstring &wide = path.native();
    if (wide.empty()) {
        return {};
    }

    int length = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0) {
        return {};
    }

    std::string utf8(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), length, nullptr, nullptr);
    return utf8;
#else
    return path.string();
#endif
}

#endif
