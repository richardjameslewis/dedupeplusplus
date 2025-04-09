#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace dedupe {

template<typename T>
class NestedNode : public std::enable_shared_from_this<NestedNode<T>> {
public:
    using NodePtr = std::shared_ptr<NestedNode<T>>;
    using NodeList = std::vector<NodePtr>;

    NestedNode(const T& data, int left = 0, int right = 0)
        : data_(data)
        , left_(left)
        , right_(right)
        , parent_(nullptr)
    {}

    // Getters
    const T& data() const { return data_; }
    T& data() { return data_; }
    int left() const { return left_; }
    int right() const { return right_; }
    const NodePtr& parent() const { return parent_; }
    const NodeList& children() const { return children_; }
    NodeList& children() { return children_; }
    size_t childCount() const { return children_.size(); }

    // Setters
    void setLeft(int left) { left_ = left; }
    void setRight(int right) { right_ = right; }
    void setParent(const NodePtr& parent) { parent_ = parent; }

    // Tree operations
    void addChild(const NodePtr& child) {
        child->setParent(this->shared_from_this());
        children_.push_back(child);
    }

    bool isLeaf() const { return children_.empty(); }
    bool isRoot() const { return parent_ == nullptr; }

    // Nested set operations
    bool contains(const NestedNode<T>& other) const {
        return left_ <= other.left_ && right_ >= other.right_;
    }

    bool isAncestorOf(const NestedNode<T>& other) const {
        return left_ < other.left_ && right_ > other.right_;
    }

    bool isDescendantOf(const NestedNode<T>& other) const {
        return other.isAncestorOf(*this);
    }

private:
    T data_;
    int left_;      // Nested set left value
    int right_;     // Nested set right value
    NodePtr parent_;
    NodeList children_;
};

} // namespace dedupe 