#pragma once

#include <QMainWindow>
#include <QTreeView>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStatusBar>
#include "filesystem_model.hpp"

namespace dedupe {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onBrowseClicked();
    void onScanClicked();
    void onPathChanged(const QString& path);
    void updateStatusMessage(const QString& message);

private:
    void setupUi();
    void updatePath(const QString& path);
    /*void setIdentical(FileSystemNode& node);*/
    
    QWidget* centralWidget_;
    QVBoxLayout* mainLayout_;
    QHBoxLayout* pathLayout_;
    QLineEdit* pathEdit_;
    QPushButton* browseButton_;
    QPushButton* scanButton_;
    QTreeView* treeView_;
    FileSystemModel* model_;
    DuplicateFinder* duplicateFinder_;
    QString currentPath_;
    QStatusBar* statusBar_;
};

} // namespace dedupe 