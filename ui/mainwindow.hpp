#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressBar>
#include <QTextEdit>
#include <QCheckBox>
#include <memory>
#include "../interface/iscanner.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void browseDirectory();
    void startScan();
    void cancelScan();
    void updateProgress(const QString& message, double progress);
    void scanCompleted(const std::vector<dedupe::DuplicateGroup>& duplicates);

private:
    void setupUi();
    void createConnections();

    QPushButton* browseButton_;
    QLineEdit* directoryEdit_;
    QPushButton* scanButton_;
    QPushButton* cancelButton_;
    QProgressBar* progressBar_;
    QTextEdit* resultsEdit_;
    QCheckBox* recursiveCheck_;
    
    std::unique_ptr<dedupe::IScanner> scanner_;
    bool isScanning_;
}; 