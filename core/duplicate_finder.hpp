#pragma once

#include "filesystem_tree.hpp"
#include "hasher.hpp"
#include "progress.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <filesystem>

namespace dedupe {

struct DuplicateSignature {
    uintmax_t size;
    std::string hash;
    int count;
    
    DuplicateSignature(uintmax_t s = 0, const std::string& h = "") 
        : size(s), hash(h), count(1) {}
};

class DuplicateFinder {
    using DuplicateMap = std::unordered_map<std::filesystem::path, DuplicateSignature>;
public:
    static DuplicateMap findDuplicates(
        const FileSystemTree& tree,
        Progress& progress,
        const std::function<bool()>& shouldCancel = []() { return false; }
    ) {
        std::vector<std::pair<std::filesystem::path, uintmax_t>> files;
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
        std::unordered_map<uintmax_t, std::vector<std::filesystem::path>> sizeGroups;
        for (const auto& [path, size] : files) {
            sizeGroups[size].push_back(path);
        }
        
        // For each size group with more than one file, compute hashes and group by hash
        std::unordered_map<std::filesystem::path, DuplicateSignature> duplicateMap;
        Hasher hasher;
        
        size_t filesToHash = 0;
        for (const auto& [size, fileGroup] : sizeGroups) {
            if (fileGroup.size() > 1) {
                filesToHash += fileGroup.size();
            }
        }
        
        progress.report("Computing file hashes...", 0.0);
        
        size_t filesHashed = 0;
        for (const auto& [size, fileGroup] : sizeGroups) {
            if (shouldCancel() || progress.is_cancelled()) {
                progress.report("Operation cancelled", 0.0);
                return duplicateMap;
            }
            
            if (fileGroup.size() <= 1) continue;
            
            // Compute hashes for all files in the size group
            std::unordered_map<std::string, std::vector<std::filesystem::path>> hashGroups;
            
            for (const auto& path : fileGroup) {
                if (shouldCancel() || progress.is_cancelled()) {
                    progress.report("Operation cancelled", 0.0);
                    return duplicateMap;
                }
                
                std::string hash = hasher.hash_file(path, progress);
                hashGroups[hash].push_back(path);
                filesHashed++;
                progress.report("Computing file hashes...", 
                               static_cast<double>(filesHashed) / filesToHash);
            }
            
            // Add all files with the same hash to the duplicate map
            for (const auto& [hash, hashGroup] : hashGroups) {
                if (hashGroup.size() > 1) {
                    // Create a signature for this group
                    DuplicateSignature signature(size, hash);
                    signature.count = hashGroup.size();
                    
                    // Add all files in the group to the map
                    for (const auto& path : hashGroup) {
                        duplicateMap[path] = signature;
                    }
                }
            }
        }
        
        progress.report("Duplicate search complete", 1.0);
        return duplicateMap;
    }

    static void decorateTree(FileSystemTree& tree, const DuplicateMap& duplicates)
    {
        tree.depthFirstTraverse([&](const auto& node) {
            auto &data = node->data();
            if (data.isDirectory) {
                data.isIdentical = node->children().size() != 0;
            } 
        });
        tree.depthFirstTraverse([&](const auto& node) {
            auto &data = node->data();
            if (!data.isDirectory) {
                data.isDuplicate = duplicates.find(data.path) != duplicates.end();
                if(data.isDuplicate) {
                    data.isIdentical = true;
                    node->parent()->data().isDuplicate = true;
                } else {
                    node->parent()->data().isIdentical = false;
                }
            }
        });
    }
};

} // namespace dedupe 