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
    
    DuplicateSignature(uintmax_t s = 0, const std::string& h = "", int c = 1) 
        : size(s), hash(h), count(c) {}
};

struct DuplicateFiles {
    std::vector<std::string> files;
    DuplicateSignature signature;

    DuplicateFiles(uintmax_t s = 0, const std::string& h = "") : signature(s, h) {}
};

using DuplicateFilesMap = std::unordered_map<std::string, DuplicateFiles>;

class DuplicateFinder {
public:
    using DuplicateMap = std::unordered_map<std::filesystem::path, DuplicateSignature>;

    //static DuplicateMap findDuplicates(
    static DuplicateFilesMap findDuplicates(
        const FileSystemTree& tree,
        Progress& progress,
        const std::function<bool()>& shouldCancel = []() { return false; }
    ) {
        //std::vector<std::pair<std::filesystem::path, uintmax_t>> files;
        //std::vector<NestedNode<FileSystemNode>&> nodes;
        std::unordered_map<std::filesystem::path, NestedNode<FileSystemNode> *> nodes;
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
                //files.emplace_back(node->data().path, node->data().size);
                //nodes.emplace_back(node);
                nodes[node->data().path] = node.get();
                //nodes.insert(std::pair(node->data().path, *node));
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
        //for (const auto& [path, size] : files) {
        for (const auto& [path, node] : nodes) {
            sizeGroups[node->data().size].push_back(path);
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
        // Compute hashes for all files in the size group
        //std::unordered_map<std::string, std::vector<std::filesystem::path>> hashGroups;
        DuplicateFilesMap hashGroups;
        for (const auto& [size, fileGroup] : sizeGroups) {
            if (shouldCancel() || progress.is_cancelled()) {
                progress.report("Operation cancelled", 0.0);
                return hashGroups;  
            }
            
            if (fileGroup.size() <= 1) continue;
            
            
            for (const auto& path : fileGroup) {
                if (shouldCancel() || progress.is_cancelled()) {
                    progress.report("Operation cancelled", 0.0);
                    //return duplicateMap;
                    return hashGroups;
                }
                
                std::string hash = hasher.hash_file(path, progress);
                if(hashGroups.find(hash) == hashGroups.end()) {
                    hashGroups[hash] = DuplicateFiles(size, hash);
                }
                // Hack - now we have a hash update the node
                nodes[path]->data().hash = hash;
                hashGroups[hash].files.push_back(path.string());
                filesHashed++;
                progress.report("Computing file hashes...", 
                               static_cast<double>(filesHashed) / filesToHash);
            }
        }
        return hashGroups;
    }

    static DuplicateMap makeDuplicateMap(const DuplicateFilesMap& hashGroups, Progress& progress) {
        DuplicateMap duplicateMap;
        // Add all files with the same hash to the duplicate map
        for (const auto& [hash, hashGroup] : hashGroups) {
            if (hashGroup.files.size() > 1) {
                // Create a signature for this group
                //DuplicateSignature signature(size, hash);
                //signature.count = hashGroup.files.size();
                
                // Add all files in the group to the map
                for (const auto& path : hashGroup.files) {
                    duplicateMap[path] = hashGroup.signature;
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
            } else {
                auto parent = node->parent();
                if (!data.isIdentical && parent != nullptr)
                    parent->data().isIdentical = false;
            }
        });
    }
};

} // namespace dedupe 