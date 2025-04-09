#pragma once

#include "nested_node.hpp"
#include <memory>
#include <vector>
#include <functional>
#include <queue>

namespace dedupe {

template<typename T>
class NestedTree {
public:
    using NodePtr = typename NestedNode<T>::NodePtr;
    using NodeList = typename NestedNode<T>::NodeList;
    using Visitor = std::function<void(const NodePtr&)>;

    NestedTree() : root_(nullptr) {}

    // Tree operations
    void setRoot(const NodePtr& root) {
        root_ = root;
        updateNestedSets();
    }

    const NodePtr& root() const { return root_; }

    // Tree traversal
    void breadthFirstTraverse(Visitor visitor) const {
        if (!root_) return;
        breadthFirstTraverseImpl(root_, visitor);
    }

    void depthFirstTraverse(Visitor visitor) const {
        if (!root_) return;
        depthFirstTraverseImpl(root_, visitor);
    }

    void levelOrderTraverse(Visitor visitor) const {
        if (!root_) return;
        std::queue<NodePtr> queue;
        queue.push(root_);
        
        while (!queue.empty()) {
            NodePtr current = queue.front();
            queue.pop();
            
            visitor(current);
            
            for (const auto& child : current->children()) {
                queue.push(child);
            }
        }
    }

    // Nested set operations
    void updateNestedSets() {
        if (!root_) return;
        int counter = 1;
        updateNestedSetsImpl(root_, counter);
    }

    // Tree queries
    NodePtr findNode(const std::function<bool(const NodePtr&)>& predicate) const {
        NodePtr result;
        breadthFirstTraverse([&](const NodePtr& node) {
            if (!result && predicate(node)) {
                result = node;
            }
        });
        return result;
    }

    std::vector<NodePtr> findAllNodes(const std::function<bool(const NodePtr&)>& predicate) const {
        std::vector<NodePtr> results;
        breadthFirstTraverse([&](const NodePtr& node) {
            if (predicate(node)) {
                results.push_back(node);
            }
        });
        return results;
    }

    // Tree transformations
    template<typename U>
    NestedTree<U> transform(const std::function<U(const T&)>& transformer) const {
        NestedTree<U> result;
        if (!root_) return result;

        auto newRoot = std::make_shared<NestedNode<U>>(transformer(root_->data()));
        transformImpl(root_, newRoot, transformer);
        result.setRoot(newRoot);
        return result;
    }

private:
    void breadthFirstTraverseImpl(const NodePtr& node, Visitor visitor) const {
        if(!node->parent()) {
            visitor(node);
        }
        for (const auto& child : node->children()) {
            visitor(child);
        }
        for (const auto& child : node->children()) {
            breadthFirstTraverseImpl(child, visitor);
        }
    }

    void depthFirstTraverseImpl(const NodePtr& node, Visitor visitor) const {
        for (const auto& child : node->children()) {
            depthFirstTraverseImpl(child, visitor);
        }
        visitor(node);
    }

    void updateNestedSetsImpl(NodePtr& node, int& counter) {
        node->setLeft(counter++);
        for (auto& child : node->children()) {
            updateNestedSetsImpl(child, counter);
        }
        node->setRight(counter++);
    }

    template<typename U>
    void transformImpl(const NodePtr& source, typename NestedNode<U>::NodePtr& target,
                      const std::function<U(const T&)>& transformer) {
        for (const auto& sourceChild : source->children()) {
            auto targetChild = std::make_shared<NestedNode<U>>(transformer(sourceChild->data()));
            target->addChild(targetChild);
            transformImpl(sourceChild, targetChild, transformer);
        }
    }

    NodePtr root_;
};

} // namespace dedupe 