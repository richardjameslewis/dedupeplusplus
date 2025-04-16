#pragma once

#include <QAbstractItemModel>
#include <QIcon>
#include "../core/filesystem_tree.hpp"
#include "../core/duplicate_finder.hpp"
#include <memory>

namespace dedupe {

class FileSystemModel : public QAbstractItemModel {
    Q_OBJECT

public:
    enum class Column {
        Name = 0,
        Size,
        Hash,
        Duplicate,
        Identical,
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
    void setDuplicates(const HashToDuplicate &duplicates);
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
    QString formatBoolean(bool value) const;
    QIcon createCompositeIcon(const QIcon& baseIcon, const QIcon& suffixIcon) const;
    QIcon createSuffixIcon(const QString& text) const;
    
    // Tooltip methods
    QString getTooltipForNode(const NestedNode<FileSystemNode>* node) const;
    
    std::unique_ptr<FileSystemTree> tree_;
    std::unique_ptr<HashToDuplicate> hashToDuplicate_;
    static const QStringList columnHeaders_;
    
    // Icon members
    QIcon baseFileIcon_;
    QIcon baseFolderIcon_;
    QIcon identicalSuffix_;
    QIcon duplicateSuffix_;
};

} // namespace dedupe 