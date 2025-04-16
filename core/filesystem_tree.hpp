#pragma once

#include "nested_tree.hpp"
#include "progress.hpp"
#include <filesystem>
#include <string>
#include <sstream>
#include <memory>
#include <functional>

namespace dedupe {

struct FileSystemNode {
    // All
    std::filesystem::path path;
    bool isDirectory;
    bool isDuplicate;
    bool isIdentical;
    uintmax_t size;

    // Files
    std::string hash;  // Empty for directories

    FileSystemNode(const std::filesystem::path& p, bool isDir = false, uintmax_t s = 0)
        : path(p)
        , isDirectory(isDir)
        , size(s)
        , isDuplicate(false)
        , isIdentical(false)
        , hash("")
    {}
};

class FileSystemTree : public NestedTree<FileSystemNode> {
public:
    using NodePtr = typename NestedTree<FileSystemNode>::NodePtr;
    using Visitor = typename NestedTree<FileSystemNode>::Visitor;

    static int errors;
    static int directoryCount;
    static int fileCount;

    // Build a tree from a filesystem path
    static FileSystemTree buildFromPath(const std::filesystem::path& rootPath,
                                      Progress& progress) {
        FileSystemTree tree;
        errors = 0;
        directoryCount = fileCount = 0;
        auto root = std::make_shared<NestedNode<FileSystemNode>>(
            FileSystemNode(rootPath, std::filesystem::is_directory(rootPath))
        );
        
        if (std::filesystem::is_directory(rootPath)) {
            ++directoryCount;
            buildDirectoryTree(root, rootPath, progress);
        } else {
            ++fileCount;
            root->data().size = std::filesystem::file_size(rootPath);
        }
        
        tree.setRoot(root);
        return tree;
    }

    // Find nodes by path
    NodePtr findByPath(const std::filesystem::path& path) const {
        return findNode([&path](const NodePtr& node) {
            return node->data().path == path;
        });
    }

    // Find all files with a specific hash
    std::vector<NodePtr> findFilesByHash(const std::string& hash) const {
        return findAllNodes([&hash](const NodePtr& node) {
            return !node->data().isDirectory && node->data().hash == hash;
        });
    }

    // Calculate total size of a subtree
    uintmax_t calculateSubtreeSize(const NodePtr& node) const {
        uintmax_t total = node->data().size;
        for (const auto& child : node->children()) {
            total += calculateSubtreeSize(child);
        }
        return total;
    }

    // Create an aggregate tree based on file hashes
    // FileSystemTree createHashAggregateTree() const {
    //     return transform<FileSystemNode>([this](const FileSystemNode& node) {
    //         if (node.isDirectory) {
    //             return FileSystemNode(node.path, true, calculateSubtreeSize(findByPath(node.path)));
    //         }
    //         return node;
    //     });
    // }

private:
    static void buildDirectoryTree(NodePtr& parent, const std::filesystem::path& dirPath,
                                 Progress& progress) {
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            try {
                int count = FileSystemTree::directoryCount + FileSystemTree::fileCount;
                std::stringstream ss;
                ss << count << " Scanning directory: " << entry.path().string();
                progress.report(ss.str(), 0.0);

                auto node = std::make_shared<NestedNode<FileSystemNode>>(
                    FileSystemNode(entry.path(), std::filesystem::is_directory(entry.path()))
                );

                if (std::filesystem::is_directory(entry.path())) {
                    ++directoryCount;
                    buildDirectoryTree(node, entry.path(), progress);
                } else {
                    ++fileCount;
                    node->data().size = std::filesystem::file_size(entry.path());
                }

                parent->addChild(node);
            }
            catch (const std::exception& e) {
                //std::cout << "Failed when scanning " << dirPath << " with " << e.what();
                //progress.report("Failed when scanning " + entry.path().string() + " with " + e.what(), 50.0);
                progress.report("Failed when scanning ", 50.0);
                //throw e;
                ++errors;
            }
        }
    }
};

} // namespace dedupe 