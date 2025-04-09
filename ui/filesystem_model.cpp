#include "filesystem_model.hpp"
#include <QFileIconProvider>
#include <QIcon>
#include <QDir>

namespace dedupe {

const QStringList FileSystemModel::columnHeaders_ = {
    "Name", "Size", "Hash"
};

FileSystemModel::FileSystemModel(QObject* parent)
    : QAbstractItemModel(parent)
    , tree_(std::make_unique<FileSystemTree>())
{
}

QModelIndex FileSystemModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!tree_->root()) return QModelIndex();

    const NestedNode<FileSystemNode>* parentNode = nullptr;
    if (parent.isValid()) {
        parentNode = static_cast<const NestedNode<FileSystemNode>*>(parent.internalPointer());
    } else {
        parentNode = tree_->root().get();
    }

    if (row < 0 || row >= static_cast<int>(parentNode->children().size())) {
        return QModelIndex();
    }

    return createIndex(row, column, parentNode->children()[row].get());
}

QModelIndex FileSystemModel::parent(const QModelIndex& child) const
{
    if (!child.isValid()) return QModelIndex();

    const NestedNode<FileSystemNode>* childNode = static_cast<const NestedNode<FileSystemNode>*>(child.internalPointer());
    const auto& parent = childNode->parent();
    
    if (!parent) return QModelIndex();

    // Find the row of the parent in its parent's children
    const auto& grandParent = parent->parent();
    if (!grandParent) {
        // Parent is root
        return createIndex(0, 0, parent.get());
    }

    const auto& siblings = grandParent->children();
    for (size_t i = 0; i < siblings.size(); ++i) {
        if (siblings[i].get() == parent.get()) {
            return createIndex(static_cast<int>(i), 0, parent.get());
        }
    }

    return QModelIndex();
}

int FileSystemModel::rowCount(const QModelIndex& parent) const
{
    if (!tree_->root()) return 0;

    if (!parent.isValid()) {
        // At root level, return number of children of root node
        return static_cast<int>(tree_->root()->children().size());
    }

    const NestedNode<FileSystemNode>* node = static_cast<const NestedNode<FileSystemNode>*>(parent.internalPointer());
    return static_cast<int>(node->children().size());
}

int FileSystemModel::columnCount(const QModelIndex&) const
{
    return static_cast<int>(Column::ColumnCount);
}

QVariant FileSystemModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !tree_->root()) return QVariant();

    const NestedNode<FileSystemNode>* node = static_cast<const NestedNode<FileSystemNode>*>(index.internalPointer());
    const auto& data = node->data();

    if (role == Qt::DisplayRole) {
        switch (static_cast<Column>(index.column())) {
            case Column::Name:
                return QString::fromStdString(data.path.filename().string());
            case Column::Size:
                return data.isDirectory ? QString() : formatSize(data.size);
            case Column::Hash:
                return data.isDirectory ? QString() : formatHash(data.hash);
            default:
                return QVariant();
        }
    }
    else if (role == Qt::DecorationRole && index.column() == 0) {
        static QFileIconProvider iconProvider;
        return iconProvider.icon(data.isDirectory ? QFileIconProvider::Folder : QFileIconProvider::File);
    }

    return QVariant();
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return columnHeaders_[section];
    }
    return QVariant();
}

void FileSystemModel::setTree(const FileSystemTree& tree)
{
    beginResetModel();
    tree_ = std::make_unique<FileSystemTree>(tree);
    endResetModel();
}

void FileSystemModel::clear()
{
    beginResetModel();
    tree_.reset();
    endResetModel();
}

FileSystemModel::NodeIndex FileSystemModel::getNodeIndex(const QModelIndex& index) const
{
    if (!index.isValid()) return NodeIndex();
    
    const NestedNode<FileSystemNode>* node = static_cast<const NestedNode<FileSystemNode>*>(index.internalPointer());
    return NodeIndex(node, index.row());
}

QString FileSystemModel::formatSize(uintmax_t bytes) const
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        ++unitIndex;
    }

    return QString("%1 %2").arg(size, 0, 'f', 1).arg(units[unitIndex]);
}

QString FileSystemModel::formatHash(const std::string& hash) const
{
    if (hash.empty()) return QString();
    return QString::fromStdString(hash.substr(0, 8) + "...");
}

} // namespace dedupe 