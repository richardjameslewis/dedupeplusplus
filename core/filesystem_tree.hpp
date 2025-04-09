#pragma once

#include "nested_tree.hpp"
#include <filesystem>
#include <string>
#include <memory>
#include <functional>

namespace dedupe {

struct FileSystemNode {
    std::filesystem::path path;
    bool isDirectory;
    uintmax_t size;
    std::string hash;  // Empty for directories

    FileSystemNode(const std::filesystem::path& p, bool isDir = false, uintmax_t s = 0)
        : path(p)
        , isDirectory(isDir)
        , size(s)
    {}
};

class FileSystemTree : public NestedTree<FileSystemNode> {
public:
    using NodePtr = typename NestedTree<FileSystemNode>::NodePtr;
    using Visitor = typename NestedTree<FileSystemNode>::Visitor;

    // Build a tree from a filesystem path
    static FileSystemTree buildFromPath(const std::filesystem::path& rootPath,
                                      std::function<void(const std::filesystem::path&)> progressCallback = nullptr) {
        FileSystemTree tree;
        auto root = std::make_shared<NestedNode<FileSystemNode>>(
            FileSystemNode(rootPath, std::filesystem::is_directory(rootPath))
        );
        
        if (std::filesystem::is_directory(rootPath)) {
            buildDirectoryTree(root, rootPath, progressCallback);
        } else {
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
                                 std::function<void(const std::filesystem::path&)>& progressCallback) {
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (progressCallback) {
                progressCallback(entry.path());
            }

            auto node = std::make_shared<NestedNode<FileSystemNode>>(
                FileSystemNode(entry.path(), std::filesystem::is_directory(entry.path()))
            );

            if (std::filesystem::is_directory(entry.path())) {
                buildDirectoryTree(node, entry.path(), progressCallback);
            } else {
                node->data().size = std::filesystem::file_size(entry.path());
            }

            parent->addChild(node);
        }
    }
};

} // namespace dedupe 