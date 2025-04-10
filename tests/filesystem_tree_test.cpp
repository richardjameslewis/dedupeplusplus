#include <gtest/gtest.h>
#include "../core/filesystem_tree.hpp"
#include "../core/progress.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

namespace dedupe {
namespace test {

class FileSystemTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory structure for testing
        tempDir_ = std::filesystem::temp_directory_path() / "filesystem_tree_test";
        std::filesystem::create_directories(tempDir_);
        
        // Create test files and directories
        auto dir1 = tempDir_ / "dir1";
        auto dir2 = tempDir_ / "dir2";
        std::filesystem::create_directories(dir1);
        std::filesystem::create_directories(dir2);

        // Create some test files
        std::ofstream(tempDir_ / "file1.txt") << "test content 1";
        std::ofstream(tempDir_ / "file2.txt") << "test content 2";
        std::ofstream(dir1 / "file3.txt") << "test content 3";
        std::ofstream(dir2 / "file4.txt") << "test content 4";
    }

    void TearDown() override {
        std::filesystem::remove_all(tempDir_);
    }

    std::filesystem::path tempDir_;
};

TEST_F(FileSystemTreeTest, BuildFromPath) {
    Progress progress;
    auto tree = FileSystemTree::buildFromPath(tempDir_, progress);
    EXPECT_TRUE(tree.root() != nullptr);
    EXPECT_EQ(tree.root()->data().path, tempDir_);
    EXPECT_TRUE(tree.root()->data().isDirectory);
    for(auto child : tree.root()->children()) {
        std::cout << child->data().path << std::endl;
    }
    EXPECT_EQ(tree.root()->childCount(), 4);  // dir1, dir2, file1.txt, and file2.txt
}

TEST_F(FileSystemTreeTest, FindByPath) {
    Progress progress;
    auto tree = FileSystemTree::buildFromPath(tempDir_, progress);
    auto node = tree.findByPath(tempDir_ / "file1.txt");
    EXPECT_TRUE(node != nullptr);
    EXPECT_FALSE(node->data().isDirectory);
    EXPECT_EQ(node->data().path, tempDir_ / "file1.txt");
}

TEST_F(FileSystemTreeTest, CalculateSubtreeSize) {
    Progress progress;
    auto tree = FileSystemTree::buildFromPath(tempDir_, progress);
    auto root = tree.root();
    uintmax_t totalSize = tree.calculateSubtreeSize(root);
    EXPECT_GT(totalSize, 0);
}

} // namespace test
} // namespace dedupe 