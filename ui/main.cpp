#include <QApplication>
#include "mainwindow.hpp"
#include "filesystem_tree.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    dedupe::MainWindow window;
    window.show();
    
    return app.exec();
} 

int dedupe::FileSystemTree::errors = 0;
int dedupe::FileSystemTree::directoryCount = 0;
int dedupe::FileSystemTree::fileCount = 0;
