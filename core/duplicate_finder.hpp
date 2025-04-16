#pragma once

#include "filesystem_tree.hpp"
#include "hasher.hpp"
#include "progress.hpp"
#include <vector>
#include <algorithm>
#include <set>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace dedupe {
    // RJL TODO
    using Hash = std::string;
    using Path = std::filesystem::path;

struct DuplicateSignature {
    uintmax_t size;
    Hash hash;
    
    DuplicateSignature(uintmax_t s = 0, const Hash & h = "")
        : size(s), hash(h) {}
};

struct DuplicateFiles {
    std::vector<std::filesystem::path> paths;
    DuplicateSignature signature;

    bool isIdentical() { return paths.size() > 1; }

    DuplicateFiles(uintmax_t s = 0, const std::string& h = "") : signature(s, h) {}
};

using HashToDuplicate = std::unordered_map<Hash, DuplicateFiles>;               // Owns DuplicateFiles


class DuplicateFinder {
    HashToDuplicate _hashToDuplicate;
    const FileSystemTree& _tree;

public:
    using DuplicateMap = std::unordered_map<std::filesystem::path, DuplicateSignature>;

    DuplicateFinder(const FileSystemTree& t) : _tree(t) { }

    HashToDuplicate hashToDuplicate() { return _hashToDuplicate; }

    bool findDuplicates(
        Progress& progress
    ) {
        size_t totalFiles = _tree.directoryCount + _tree.fileCount;
        
        progress.report("Collecting file information...", 0.0);

        std::unordered_map<uintmax_t, std::vector<NestedNode<FileSystemNode> *>> sizeGroups;
        _tree.depthFirstTraverse([&](const auto& node) {
            const auto& data = node->data();
            if (progress.is_cancelled()) {
                progress.report("Operation cancelled", 0.0);
                return;
            }
            if (!data.isDirectory) {
                if (sizeGroups.find(data.size) == sizeGroups.end())
                    sizeGroups[data.size] = std::vector<NestedNode<FileSystemNode> *>();
                sizeGroups[data.size].push_back(node.get());
            }
        });

        int possible = 0;
        for (auto& [size, fileGroup] : sizeGroups) {
            int count = fileGroup.size();
            possible += count > 1 ? count : 0;
        }

        // For each size group with more than one file, compute hashes and group by hash
        Hasher hasher;
        int quickHashed = 0;
        std::set<std::filesystem::path> hashFiles;
        size_t filesToHash = 0;
        for (auto& [size, fileGroup] : sizeGroups) {
            if (progress.is_cancelled()) {
                progress.report("Operation cancelled", 0.0);
                return false;
            }
            if (fileGroup.size() > 1) {
                filesToHash += fileGroup.size();
                std::set<Hash> unique{};
                bool bFullHash = false;
                for (auto f : fileGroup) {
                    try {
                        std::stringstream ss;
                        ss << quickHashed << "/" << possible << "/" << (_tree.directoryCount + _tree.fileCount) << " Quick hash: " << f->data().path.filename().string();
                        progress.report(ss.str(), 0.0);
                        f->data().hash = hasher.hash_file(f->data().path, progress, true);
                        if (unique.find(f->data().hash) != unique.end()) {
                            // At least one quick hash is equal to another: bail & compute long hashes
                            bFullHash = true;
                            break;
                        }
                        else {
                            // Not found: add to set
                            unique.insert(f->data().hash);
                        }
                    }
                    catch (const std::exception& e) {
                        std::stringstream ss;
                        ss << "Failed quick hashing " << size << " " << f->data().path.string() << " with " << e.what();
                        progress.report(ss.str(), 0.0);
                    }

                }
                if (bFullHash) {
                    for (auto f : fileGroup) {
                        // reset any hash set above
                        f->data().hash = "";
                        // and add these files to the list of those to get a full file hash
                        hashFiles.emplace(f->data().path);
                    }
                }
                else {
                    quickHashed += fileGroup.size();
                    possible -= fileGroup.size();
                }
            }

        }

        progress.report("Computing file hashes...", 0.0);
        int soFar = 0;
        int hashed = 0;
        _tree.depthFirstTraverse([&](const auto& node) {
            try {
                if (progress.is_cancelled()) {
                    progress.report("Operation cancelled", 0.0);
                    return;
                }
                auto& data = node->data();
                if (data.isDirectory) {
                    std::vector<std::string> hashes;
                    for (auto child : node->children()) {
                        hashes.push_back(child->data().hash);
                    }
                    std::sort(hashes.begin(), hashes.end());
                    std::string signature = "";
                    for (auto h : hashes) {
                        signature += (signature != "" ? ", " : "") + h;
                    }
                    data.size = node->children().size();
                    data.hash = Hasher::hash_string(signature, progress);
                }
                else {
                    if (hashFiles.find(data.path) != hashFiles.end()) {
                        // Hash whole file
                        std::stringstream ss;
                        ss << hashed << "/" << soFar << "/" << (_tree.directoryCount + _tree.fileCount) << " Hashing: " << data.path.filename().string() << " ";
                        progress.report(ss.str(), 50.0);
                        data.hash = hasher.hash_file(data.path, progress);
                        ++hashed;
                    }
                    else {
                        // If an existing hash doesn't exist fake a hash from the file size
                        if(data.hash == "")
                            data.hash = Hasher::fake_size_hash(data.size);
                    }
                }
                if (_hashToDuplicate.find(data.hash) == _hashToDuplicate.end()) {
                    _hashToDuplicate[data.hash] = DuplicateFiles(data.size, data.hash);
                }
                _hashToDuplicate[data.hash].paths.push_back(data.path);
                ++soFar;
            }
            catch (const std::exception& e) {
                std::stringstream ss;
                ss << "Failed when hashing " << node->data().path << " with " << e.what();
                progress.report(ss.str(), 50.0);
            }
        });


        _tree.depthFirstTraverse([&](const auto& node) {
            if (progress.is_cancelled()) {
                progress.report("Operation cancelled", 0.0);
                return;
            }
            auto& data = node->data();
            auto& dupe = _hashToDuplicate[data.hash];
            data.isIdentical = dupe.isIdentical();
            if (data.isDirectory && !data.isIdentical) {
                data.isDuplicate = false;
                for (auto& c : node->children()) {
                    if (c->data().isDuplicate) {
                        data.isDuplicate = true;
                        break;
                    }
                }
            }
            else {
                data.isDuplicate = data.isIdentical;
            }
        });
        return true;
    }
};

} // namespace dedupe 