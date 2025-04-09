#include "mainwindow.hpp"
#include "../interface/scanner_impl.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QFuture>
#include <QtConcurrent>
#include "../core/filesystem_tree.hpp"

namespace dedupe {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , centralWidget_(new QWidget(this))
    , mainLayout_(new QVBoxLayout(centralWidget_))
    , pathLayout_(new QHBoxLayout())
    , pathEdit_(new QLineEdit(this))
    , browseButton_(new QPushButton("Browse...", this))
    , scanButton_(new QPushButton("Scan", this))
    , treeView_(new QTreeView(this))
    , model_(new FileSystemModel(this))
{
    setCentralWidget(centralWidget_);
    setupUi();
    
    // Connect signals
    connect(browseButton_, &QPushButton::clicked, this, &MainWindow::onBrowseClicked);
    connect(scanButton_, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(pathEdit_, &QLineEdit::textChanged, this, &MainWindow::onPathChanged);
    
    updatePath("c:\\todo\\peel sessions");

    // Set up tree view
    treeView_->setModel(model_);
    treeView_->setAlternatingRowColors(true);
    treeView_->setSortingEnabled(true);
    treeView_->setAnimated(true);
    treeView_->setIndentation(20);
    treeView_->setUniformRowHeights(true);
    treeView_->setColumnWidth(0, 500);
    treeView_->setColumnWidth(1, 50);
    treeView_->setColumnWidth(2, 100);
    
    // Set window properties
    setWindowTitle("Dedupe++");
    resize(800, 600);
}

void MainWindow::setupUi()
{
    // Path selection layout
    pathLayout_->addWidget(new QLabel("Directory:", this));
    pathLayout_->addWidget(pathEdit_);
    pathLayout_->addWidget(browseButton_);
    pathLayout_->addWidget(scanButton_);
    
    // Main layout
    mainLayout_->addLayout(pathLayout_);
    mainLayout_->addWidget(treeView_);
}

void MainWindow::onBrowseClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory",
                                                  currentPath_.isEmpty() ? QDir::homePath() : currentPath_,
                                                  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!dir.isEmpty()) {
        updatePath(dir);
    }
}

void MainWindow::onScanClicked()
{
    if (currentPath_.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select a directory first.");
        return;
    }
    
    try {
            auto tree = FileSystemTree::buildFromPath(currentPath_.toStdString());
        model_->setTree(tree);
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Failed to scan directory: %1").arg(e.what()));
    }
}

void MainWindow::onPathChanged(const QString& path)
{
    updatePath(path);
}

void MainWindow::updatePath(const QString& path)
{
    currentPath_ = path;
    pathEdit_->setText(path);
}

} // namespace dedupe 