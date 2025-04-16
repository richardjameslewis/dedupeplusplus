#include <gtest/gtest.h>
#include "../core/nested_tree.hpp"
#include "filesystem_tree.hpp"
#include <string>
#include <vector>
#include <iostream>

namespace dedupe {

int dedupe::FileSystemTree::errors = 0;
int dedupe::FileSystemTree::directoryCount = 0;
int dedupe::FileSystemTree::fileCount = 0;
   
namespace test {

    // Test data structure for basic tree tests
struct TestNode {
    int value;
    TestNode(int v) : value(v) {}
};

class TreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple test tree:
        //      1
        //    / | \
        //   2  3  4
        //  /   |
        // 5    6
        auto root = std::make_shared<NestedNode<TestNode>>(TestNode(1));
        auto node2 = std::make_shared<NestedNode<TestNode>>(TestNode(2));
        auto node3 = std::make_shared<NestedNode<TestNode>>(TestNode(3));
        auto node4 = std::make_shared<NestedNode<TestNode>>(TestNode(4));
        auto node5 = std::make_shared<NestedNode<TestNode>>(TestNode(5));
        auto node6 = std::make_shared<NestedNode<TestNode>>(TestNode(6));

        node2->addChild(node5);
        node3->addChild(node6);
        root->addChild(node2);
        root->addChild(node3);
        root->addChild(node4);

        tree_.setRoot(root);
    }

    NestedTree<TestNode> tree_;
};

TEST_F(TreeTest, BasicTreeOperations) {
    EXPECT_TRUE(tree_.root() != nullptr);
    EXPECT_EQ(tree_.root()->data().value, 1);
    EXPECT_EQ(tree_.root()->childCount(), 3);
    EXPECT_TRUE(tree_.root()->children()[0]->data().value == 2);
    EXPECT_TRUE(tree_.root()->children()[1]->data().value == 3);
    EXPECT_TRUE(tree_.root()->children()[2]->data().value == 4);
}

TEST_F(TreeTest, NestedSetOperations) {
    auto root = tree_.root();
    auto node2 = root->children()[0];
    auto node3 = root->children()[1];
    auto node4 = root->children()[2];
    auto node5 = node2->children()[0];
    auto node6 = node3->children()[0];

    EXPECT_EQ(node2->data().value, 2);
    EXPECT_EQ(node3->data().value, 3);
    EXPECT_EQ(node4->data().value, 4);
    EXPECT_EQ(node5->data().value, 5);
    EXPECT_EQ(node6->data().value, 6);

    // Check nested set relationships
    EXPECT_TRUE(root->contains(*node2));
    EXPECT_TRUE(root->contains(*node3));
    EXPECT_TRUE(root->contains(*node4));
    EXPECT_TRUE(root->contains(*node5));
    EXPECT_TRUE(root->contains(*node6));
    EXPECT_TRUE(node2->contains(*node5));
    EXPECT_TRUE(node3->contains(*node6));
    EXPECT_FALSE(node2->contains(*node3));
    EXPECT_FALSE(node3->contains(*node4));
}

TEST_F(TreeTest, TreeTraversal) {
    std::vector<int> preOrderValues;
    tree_.breadthFirstTraverse([&](const auto& node) {
        preOrderValues.push_back(node->data().value);
    });
    EXPECT_EQ(preOrderValues, std::vector<int>({1, 2, 3, 4, 5, 6}));
    

    std::vector<int> postOrderValues;
    tree_.depthFirstTraverse([&](const auto& node) {
        std::cout << node->data().value << " ";
        postOrderValues.push_back(node->data().value);
    });
    std::cout << std::endl;
    EXPECT_EQ(postOrderValues, std::vector<int>({5, 2, 6, 3, 4, 1}));

    std::vector<int> levelOrderValues;
    tree_.levelOrderTraverse([&](const auto& node) {
        levelOrderValues.push_back(node->data().value);
    });
    EXPECT_EQ(levelOrderValues, std::vector<int>({1, 2, 3, 4, 5, 6}));
}

TEST_F(TreeTest, TreeQueries) {
    auto node = tree_.findNode([](const auto& n) { return n->data().value == 3; });
    EXPECT_TRUE(node != nullptr);
    EXPECT_EQ(node->data().value, 3);

    auto nodes = tree_.findAllNodes([](const auto& n) { return n->data().value > 3; });
    EXPECT_EQ(nodes.size(), 3);  // Should find nodes 4, 5, and 6
}

//  TEST_F(TreeTest, TreeTransformation) {
//      auto transformed = tree_.transform<std::string>([](const TestNode& node) {
//          return std::to_string(node.value);
//      });

//      std::vector<std::string> values;
//      transformed.breadthFirstTraverse([&](const auto& node) {
//          values.push_back(node->data());
//      });
//      EXPECT_EQ(values, std::vector<std::string>({"1", "2", "5", "3", "6", "4"}));
// }

/*
class FileSystemTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory structure for testing
        tempDir_ = std::filesystem::temp_directory_path() / "dedupe_test";
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
    auto tree = FileSystemTree::buildFromPath(tempDir_);
    EXPECT_TRUE(tree.root() != nullptr);
    EXPECT_EQ(tree.root()->data().path, tempDir_);
    EXPECT_TRUE(tree.root()->data().isDirectory);
    EXPECT_EQ(tree.root()->childCount(), 4);  // dir1, dir2, file1.txt, and file2.txt
}

TEST_F(FileSystemTreeTest, FindByPath) {
    auto tree = FileSystemTree::buildFromPath(tempDir_);
    auto node = tree.findByPath(tempDir_ / "file1.txt");
    EXPECT_TRUE(node != nullptr);
    EXPECT_FALSE(node->data().isDirectory);
    EXPECT_EQ(node->data().path, tempDir_ / "file1.txt");
}

TEST_F(FileSystemTreeTest, CalculateSubtreeSize) {
    auto tree = FileSystemTree::buildFromPath(tempDir_);
    auto root = tree.root();
    uintmax_t totalSize = tree.calculateSubtreeSize(root);
    EXPECT_GT(totalSize, 0);
}
*/

} // namespace test
} // namespace dedupe 