#include "filesystem_model.hpp"
#include <QFileIconProvider>
#include <QIcon>
#include <QDir>
#include <QPainter>
#include <QPixmap>
#include <QFont>

namespace dedupe {

const QStringList FileSystemModel::columnHeaders_ = {
    "Name", "Size", "Hash", "Duplicate", "Identical"
};

FileSystemModel::FileSystemModel(QObject* parent)
    : QAbstractItemModel(parent)
    , tree_(std::make_unique<FileSystemTree>())
{
 // Create base icons
    static QFileIconProvider iconProvider;
    baseFileIcon_ = iconProvider.icon(QFileIconProvider::File);
    baseFolderIcon_ = iconProvider.icon(QFileIconProvider::Folder);
    
 // Create suffix icons programmatically
    identicalSuffix_ = createSuffixIcon("=");
    duplicateSuffix_ = createSuffixIcon("D");
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
            case Column::Duplicate:
                return formatBoolean(data.isDuplicate);
            case Column::Identical:
                return formatBoolean(data.isIdentical);
            default:
                return QVariant();
        }
    }
    else if (role == Qt::DecorationRole && index.column() == 0) {
       QIcon baseIcon = data.isDirectory ? baseFolderIcon_ : baseFileIcon_;
        
        if (data.isIdentical) {
            return createCompositeIcon(baseIcon, identicalSuffix_);
        } else if (data.isDuplicate) {
            return createCompositeIcon(baseIcon, duplicateSuffix_);
        }
        
        return baseIcon;    
    }
    else if (role == Qt::ForegroundRole) {
        if (index.column() == static_cast<int>(Column::Duplicate) && data.isDuplicate) {
            return QColor(Qt::red);
        }
        if (index.column() == static_cast<int>(Column::Identical) && data.isIdentical) {
            return QColor(Qt::green);
        }
    }
    else if (role == Qt::ToolTipRole) {
        return getTooltipForNode(node);
    }

    return QVariant();
}

QString FileSystemModel::getTooltipForNode(const NestedNode<FileSystemNode>* node) const
{
    if (!node) return QString();
    
    const auto& data = node->data();
    
    if (data.isDirectory) {
        QString tooltip = QString::fromStdString("Directory ");
        if (node->data().isIdentical)
            tooltip += "all duplicates ";
        else if (node->data().isDuplicate)
            tooltip += "containing duplicates ";
        tooltip += QString::fromStdString(data.path.string());
        return tooltip;
    }

    std::vector<std::string> duplicateFilenames { data.path.string() };
    if(duplicates_ !=  nullptr) {
        const auto& it = duplicates_->find(data.hash);
        if(it != duplicates_->cend()) {
            duplicateFilenames = it->second.files;
        }
    }
    
    if (duplicateFilenames.size() <= 1) {
        return "File " + QString::fromStdString(data.path.string());
    }
    
    // Format the tooltip with duplicate filenames
    std::sort(duplicateFilenames.begin(), duplicateFilenames.end());
    QString tooltip = QString::fromStdString("Duplicate files:\n");
    size_t count = duplicateFilenames.size();
    for (size_t i = 0; i < count; i++)
    {
        tooltip += QString::fromStdString(duplicateFilenames[i]);
        if(i < count - 1) {
            tooltip += "\n";
        }
    }
    
    
    return tooltip;
}

QIcon FileSystemModel::createCompositeIcon(const QIcon& baseIcon, const QIcon& suffixIcon) const
{
    QPixmap basePixmap = baseIcon.pixmap(16, 16);
    QPixmap suffixPixmap = suffixIcon.pixmap(8, 8);
    
    QPixmap result(basePixmap.size());
    result.fill(Qt::transparent);
    
    QPainter painter(&result);
    painter.drawPixmap(0, 0, basePixmap);
    painter.drawPixmap(basePixmap.width() - suffixPixmap.width(), 
                      basePixmap.height() - suffixPixmap.height(), 
                      suffixPixmap);
    painter.end();
    
    return QIcon(result);
}

QIcon FileSystemModel::createSuffixIcon(const QString& text) const
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Set up font
    QFont font = painter.font();
    auto fontSize = text == "=" ? 16 : 12;
    font.setPointSize(fontSize);
    font.setBold(true);
    painter.setFont(font);
    
    // Draw text in white
    painter.setPen(Qt::black);
    painter.drawText(pixmap.rect(), Qt::AlignBottom, text);
    
    painter.end();
    return QIcon(pixmap);
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

void FileSystemModel::setDuplicates(const DuplicateFilesMap &duplicates)
{
    duplicates_ = std::make_unique<DuplicateFilesMap>(duplicates);
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
    
    // Format hash as a shortened version for display
    if (hash.length() > 8) {
        return QString::fromStdString(hash.substr(0, 8) + "...");
    }
    
    return QString::fromStdString(hash);
}

QString FileSystemModel::formatBoolean(bool value) const
{
    return value ? "Yes" : "No";
}

} // namespace dedupe 