#include "scanner.hpp"
#include "hasher.hpp"
#include <algorithm>

namespace dedupe {

std::vector<DuplicateGroup> Scanner::scan_directory(const std::filesystem::path& directory,
                                                  Progress& progress) {
    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
        throw std::runtime_error("Invalid directory: " + directory.string());
    }

    // First phase: Group files by size
    std::unordered_map<std::uintmax_t, std::vector<std::filesystem::path>> size_groups;
    scan_directory_recursive(directory, size_groups, progress);

    // Second phase: Hash files with same size
    return process_size_groups(size_groups, progress);
}

void Scanner::scan_directory_recursive(const std::filesystem::path& directory,
                                    std::unordered_map<std::uintmax_t, std::vector<std::filesystem::path>>& size_groups,
                                    Progress& progress) {
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (progress.is_cancelled()) {
            return;
        }

        if (entry.is_directory()) {
            if (recursive_) {
                scan_directory_recursive(entry.path(), size_groups, progress);
            }
            continue;
        }

        if (!entry.is_regular_file()) {
            continue;
        }

        try {
            auto file_size = entry.file_size();
            size_groups[file_size].push_back(entry.path());
        } catch (const std::exception& e) {
            // Skip files that can't be processed
            continue;
        }
    }
}

std::vector<DuplicateGroup> Scanner::process_size_groups(
    const std::unordered_map<std::uintmax_t, std::vector<std::filesystem::path>>& size_groups,
    Progress& progress) {
    
    std::vector<DuplicateGroup> result;
    std::unordered_map<std::string, DuplicateGroup> hash_groups;

    // Only process groups with more than one file
    for (const auto& [size, files] : size_groups) {
        if (files.size() <= 1) continue;

        // Hash files with same size
        for (const auto& file : files) {
            if (progress.is_cancelled()) {
                return result;
            }

            try {
                std::string hash = Hasher::hash_file(file, progress);
                if (!hash.empty()) {
                    hash_groups[hash].hash = hash;
                    hash_groups[hash].files.push_back(file);
                }
            } catch (const std::exception& e) {
                // Skip files that can't be processed
                continue;
            }
        }

        // Add groups with duplicates to result
        for (auto& [hash, group] : hash_groups) {
            if (group.files.size() > 1) {
                result.push_back(std::move(group));
            }
        }
        hash_groups.clear();
    }

    return result;
}

} // namespace dedupe 