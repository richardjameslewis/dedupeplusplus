#pragma once

#include "filesystem_tree.hpp"
#include "hasher.hpp"
#include "progress.hpp"
#include <vector>
#include <unordered_map>
#include <memory>

namespace dedupe {

struct FileInfo {
    std::filesystem::path path;
    uintmax_t size;
    std::string hash;
    
    FileInfo(const std::filesystem::path& p, uintmax_t s) 
        : path(p), size(s), hash("") {}
};

struct DuplicateGroup {
    uintmax_t size;
    std::vector<FileInfo> files;
    
    DuplicateGroup(uintmax_t s) : size(s) {}
};

class DuplicateFinder {
public:
    static std::vector<DuplicateGroup> findDuplicates(
        const FileSystemTree& tree,
        Progress& progress,
        const std::function<bool()>& shouldCancel = []() { return false; }
    ) {
        std::vector<FileInfo> files;
        size_t totalFiles = 0;
        
        // First pass: count total files and collect file info
        tree.breadthFirstTraverse([&](const auto& node) {
            if (!node->data().isDirectory) {
                totalFiles++;
            }
        });
        
        progress.report("Collecting file information...", 0.0);
        
        // Second pass: collect files and their sizes
        size_t filesCollected = 0;
        tree.breadthFirstTraverse([&](const auto& node) {
            if (shouldCancel() || progress.is_cancelled()) {
                progress.report("Operation cancelled", 0.0);
                return;
            }
            
            if (!node->data().isDirectory) {
                files.emplace_back(node->data().path, node->data().size);
                filesCollected++;
                progress.report("Collecting file information...", 
                               static_cast<double>(filesCollected) / totalFiles);
            }
        });
        
        if (shouldCancel() || progress.is_cancelled()) {
            return {};
        }
        
        // Group files by size
        std::unordered_map<uintmax_t, std::vector<FileInfo>> sizeGroups;
        for (auto& file : files) {
            sizeGroups[file.size].push_back(std::move(file));
            std::cout << "file: " << file.path << " size: " << file.size << std::endl;
        }
        
        // For each size group with more than one file, compute hashes and group by hash
        std::vector<DuplicateGroup> duplicateGroups;
        
        size_t filesToHash = 0;
        for (const auto& [size, fileGroup] : sizeGroups) {
            if (fileGroup.size() > 1) {
                filesToHash += fileGroup.size();
            }
        }
        
        progress.report("Computing file hashes...", 0.0);
        
        size_t filesHashed = 0;
        for (auto& [size, fileGroup] : sizeGroups) {
            if (shouldCancel() || progress.is_cancelled()) {
                progress.report("Operation cancelled", 0.0);
                return duplicateGroups;
            }
            
            if (fileGroup.size() <= 1) continue;
            
            // Compute hashes for all files in the size group
            std::unordered_map<std::string, std::vector<FileInfo>> hashGroups;
            
            for (auto& file : fileGroup) {
                if (shouldCancel() || progress.is_cancelled()) {
                    progress.report("Operation cancelled", 0.0);
                    return duplicateGroups;
                }
                
                file.hash = Hasher::hash_file(file.path, progress);
                hashGroups[file.hash].push_back(file);
                filesHashed++;
                progress.report("Computing file hashes...", 
                               static_cast<double>(filesHashed) / filesToHash);
            }
            
            // Create duplicate groups for files with the same hash
            for (auto& [hash, hashGroup] : hashGroups) {
                if (hashGroup.size() > 1) {
                    DuplicateGroup group(size);
                    group.files = std::move(hashGroup);
                    duplicateGroups.push_back(std::move(group));
                }
            }
        }
        
        progress.report("Duplicate search complete", 1.0);
        return duplicateGroups;
    }
};

} // namespace dedupe 