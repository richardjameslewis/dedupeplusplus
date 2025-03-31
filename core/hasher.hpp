#pragma once

#include <string>
#include <filesystem>
#include "progress.hpp"

namespace dedupe {

class Hasher {
public:
    // Calculate SHA-256 hash of a file
    static std::string hash_file(const std::filesystem::path& file_path,
                               Progress& progress);

    // Calculate hash of file content
    static std::string hash_content(const std::filesystem::path& file_path,
                                  Progress& progress);

private:
    static constexpr std::size_t BUFFER_SIZE = 8192; // 8KB buffer for reading
};

} // namespace dedupe 