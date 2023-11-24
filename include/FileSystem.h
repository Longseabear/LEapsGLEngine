#pragma once

#include <iostream>
#include <filesystem>
#include <core/Type.h>
namespace fs = std::filesystem;

namespace LEapsGL{
    struct FileSystem {
        inline static auto getPath(const fs::path& filePath) {
            return PathString(filePath.string().c_str());
        }
        template <typename... pathes>
        inline static auto join(const pathes&... args) {
            return getPath((fs::path(args) / ...));
        }
    };
}
