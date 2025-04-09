#include <gtest/gtest.h>
#include "../core/nested_tree.hpp"
#include <string>
#include <vector>

namespace dedupe {
namespace test {

struct TestNode {
    std::string data;
    TestNode(const std::string& d) : data(d) {}
};

TEST(NestedTreeTest, BasicOperations) {
    NestedTree<TestNode> tree;
    auto root = std::make_shared<NestedNode<TestNode>>(TestNode("root"));
    auto child1 = std::make_shared<NestedNode<TestNode>>(TestNode("child1"));
    auto child2 = std::make_shared<NestedNode<TestNode>>(TestNode("child2"));
    
    root->addChild(child1);
    root->addChild(child2);
    tree.setRoot(root);
    
    EXPECT_EQ(tree.root(), root);
    EXPECT_EQ(root->children().size(), 2);
}

TEST(NestedTreeTest, NestedSetOperations) {
    NestedTree<TestNode> tree;
    auto root = std::make_shared<NestedNode<TestNode>>(TestNode("root"));
    auto child1 = std::make_shared<NestedNode<TestNode>>(TestNode("child1"));
    auto child2 = std::make_shared<NestedNode<TestNode>>(TestNode("child2"));
    
    root->addChild(child1);
    root->addChild(child2);
    tree.setRoot(root);
    
    EXPECT_EQ(root->left(), 1);
    EXPECT_EQ(root->right(), 6);
    EXPECT_EQ(child1->left(), 2);
    EXPECT_EQ(child1->right(), 3);
    EXPECT_EQ(child2->left(), 4);
    EXPECT_EQ(child2->right(), 5);
}

TEST(NestedTreeTest, TreeTraversal) {
    NestedTree<TestNode> tree;
    auto root = std::make_shared<NestedNode<TestNode>>(TestNode("root"));
    auto child1 = std::make_shared<NestedNode<TestNode>>(TestNode("child1"));
    auto child2 = std::make_shared<NestedNode<TestNode>>(TestNode("child2"));
    
    root->addChild(child1);
    root->addChild(child2);
    tree.setRoot(root);
    
    std::vector<std::string> visited;
    
    // Test breadth-first traversal
    tree.breadthFirstTraverse([&](const auto& node) {
        visited.push_back(node->data().data);
    });
    
    EXPECT_EQ(visited.size(), 3);
    EXPECT_EQ(visited[0], "root");
    EXPECT_EQ(visited[1], "child1");
    EXPECT_EQ(visited[2], "child2");
    
    visited.clear();
    
    // Test depth-first traversal
    tree.depthFirstTraverse([&](const auto& node) {
        visited.push_back(node->data().data);
    });
    
    EXPECT_EQ(visited.size(), 3);
    EXPECT_EQ(visited[0], "child1");
    EXPECT_EQ(visited[1], "child2");
    EXPECT_EQ(visited[2], "root");
}

TEST(NestedTreeTest, TreeQueries) {
    NestedTree<TestNode> tree;
    auto root = std::make_shared<NestedNode<TestNode>>(TestNode("root"));
    auto child1 = std::make_shared<NestedNode<TestNode>>(TestNode("child1"));
    auto child2 = std::make_shared<NestedNode<TestNode>>(TestNode("child2"));
    
    root->addChild(child1);
    root->addChild(child2);
    tree.setRoot(root);
    
    auto found = tree.findNode([](const auto& node) {
        return node->data().data == "child1";
    });
    
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->data().data, "child1");
    
    auto all = tree.findAllNodes([](const auto& node) {
        return node->data().data.find("child") != std::string::npos;
    });
    
    EXPECT_EQ(all.size(), 2);
}

TEST(NestedTreeTest, TreeTransformation) {
    NestedTree<TestNode> tree;
    auto root = std::make_shared<NestedNode<TestNode>>(TestNode("root"));
    auto child1 = std::make_shared<NestedNode<TestNode>>(TestNode("child1"));
    auto child2 = std::make_shared<NestedNode<TestNode>>(TestNode("child2"));
    
    root->addChild(child1);
    root->addChild(child2);
    tree.setRoot(root);
    
    auto transformed = tree.transform<std::string>([](const TestNode& node) {
        return node.data + "_transformed";
    });
    
    std::vector<std::string> visited;
    transformed.breadthFirstTraverse([&](const auto& node) {
        visited.push_back(node->data());
    });
    
    EXPECT_EQ(visited.size(), 3);
    EXPECT_EQ(visited[0], "root_transformed");
    EXPECT_EQ(visited[1], "child1_transformed");
    EXPECT_EQ(visited[2], "child2_transformed");
}

} // namespace test
} // namespace dedupe 