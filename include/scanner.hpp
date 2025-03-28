#pragma once

#include <filesystem>
#include <vector>
#include <unordered_map>
#include "progress.hpp"

namespace dedupe {

struct DuplicateGroup {
    std::string hash;
    std::vector<std::filesystem::path> files;
};

class Scanner {
public:
    Scanner(bool recursive = true)
        : recursive_(recursive)
    {}

    std::vector<DuplicateGroup> scan_directory(const std::filesystem::path& directory,
                                             Progress& progress);

private:
    void scan_directory_recursive(const std::filesystem::path& directory,
                                std::unordered_map<std::uintmax_t, std::vector<std::filesystem::path>>& size_groups,
                                Progress& progress);

    std::vector<DuplicateGroup> process_size_groups(
        const std::unordered_map<std::uintmax_t, std::vector<std::filesystem::path>>& size_groups,
        Progress& progress);

    bool recursive_;
};

} // namespace dedupe 