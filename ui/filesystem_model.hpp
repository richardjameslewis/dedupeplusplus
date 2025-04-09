#pragma once

#include <QAbstractItemModel>
#include "../core/filesystem_tree.hpp"
#include <memory>

namespace dedupe {

class FileSystemModel : public QAbstractItemModel {
    Q_OBJECT

public:
    enum class Column {
        Name = 0,
        Size,
        Hash,
        ColumnCount
    };

    explicit FileSystemModel(QObject* parent = nullptr);

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Custom methods
    void setTree(const FileSystemTree& tree);
    void clear();

private:
    struct NodeIndex {
        const NestedNode<FileSystemNode>* node;
        int row;
        NodeIndex(const NestedNode<FileSystemNode>* n = nullptr, int r = 0) 
            : node(n), row(r) {}
    };

    NodeIndex getNodeIndex(const QModelIndex& index) const;
    QString formatSize(uintmax_t bytes) const;
    QString formatHash(const std::string& hash) const;

    std::unique_ptr<FileSystemTree> tree_;
    static const QStringList columnHeaders_;
};

} // namespace dedupe 